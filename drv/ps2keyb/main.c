/*
MIT license
Copyright (c) 2022 Maarten Vermeulen

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


// includes from the syslib
#include "kernel.h"
#include "api.h"
#include "exit_code.h"
#include "driver.h"
#include "program.h"

#define PROGRAM_NAME "PS2KEYB.DRV"
#include "debug.h"

// includes from ps2keyb
#include "main.h"
#include "ps2keyb.h"
#include "tables.h"

#define KEYB_INT                0x21
#define KEYB_PORT               0x60
#define COMMAND_PORT            0x64

#define COMMAND_PORT_OBF        0x01 // output buffer full
#define COMMAND_PORT_READY      (0x01 | 0x02)

#define CMD_SET_LEDS            0xED
#define CMD_ECHO                0xEE
#define CMD_GET_SET_SCANCODE    0xF0
#define CMD_KEYB_IDENTIFY       0xF2
#define CMD_ENABLE_SCANNING     0xF4
#define CMD_DISABLE_SCANNING    0xF5
#define CMD_SET_DEFAULT_PARAM   0xF6
#define CMD_RESEND_LAST_BYTE    0xFE
#define CMD_RESET_SELF_TEST     0xFF

#define RESP_DETECT_ERROR1      0x00
#define RESP_SELF_TEST_PASSED   0xAA
#define RESP_ECHO               0xEE
#define RESP_ACK                0xFA
#define RESP_SELF_TEST_FAIL1    0xFC
#define RESP_SELF_TEST_FAIL2    0xFD
#define RESP_RESEND             0xFE
#define RESP_DETECT_ERROR2      0xFF

#define STATE_INIT                      0
#define STATE_WAIT_FOR_RESPONSE         1
#define STATE_IDLE                      2
#define STATE_WAIT_FOR_NEXT_SCAN_CODE   3
#define STATE_ERR                       4

#define FLAG_CMD_COMPLETE       (1u << 0u)
#define FLAG_ACK                (1u << 1u)
#define FLAG_CAPS_ON            (1u << 2u) // TODO
#define FLAG_NUM_ON             (1u << 3u) // TODO
#define FLAG_SCROLL_ON          (1u << 4u) // TODO
#define FLAG_RESP_RECEIVED      (1u << 5u)
#define FLAG_KEY_RELEASED       (1u << 6u)
#define FLAG_KEY_EXTENDED       (1u << 7u)
#define FLAG_CHECK_FOR_PAUSE    (1u << 8u)

#define SCANCODE_EXTENDED_SET       0xE0u
#define SCANCODE_RELEASED           0x80u
#define SCANCDOE_EXTENDED_OFFSET    0x10u
#define SCANCODE_PAUSE_START        0xE1u

#define SUBSCRIBER_STRUCT_SPACE     4096 // bytes
#define MAX_SUBSCRIBERS             SUBSCRIBER_STRUCT_SPACE / sizeof(subscribers_t)

typedef struct ps2keyb_api_req
{
    syscall_hdr_t hdr;
    uint16_t *buffer;
    size_t buffer_size;
} __attribute__((packed)) ps2keyb_api_req;

typedef struct subscribers_t
{
    uint16_t *buffer;
    size_t buffer_nwords;
    uint32_t index;
} __attribute__((packed)) subscribers_t;

uint8_t g_state = STATE_INIT;
uint16_t g_flags = 0;
uint8_t g_response = 0;
uint16_t g_last_key = KEYCODE_UNUSED;
subscribers_t *g_subscribers = NULL;

uint8_t inb(uint16_t _port)
{
	uint8_t rv;
	__asm__ __volatile__ ("inb %1, %0" : "=a" (rv) : "dN" (_port) );
	return rv;
}

void outb (uint16_t _port, uint8_t _data)
{
	__asm__ __volatile__ ("outb %1, %0" :: "dN" (_port), "a" (_data) );
}

static uint8_t not_in(uint8_t *bfr, size_t bfr_size, uint8_t byte)
{
    for(uint32_t i = 0; i < bfr_size; ++i)
        if(bfr[i] == byte)
            return 0;
    
    return 1;
}

static uint8_t ps2keyb_subscriber_empty(uint16_t *bfr)
{
    // here I'm going to be lazy and say that if the first index is empty, we 
    // can assume the entire buffer is empty
    return bfr[0] == 0;
}

void ps2keyb_send_keycode(uint16_t keycode)
{
    g_last_key = keycode;
    for(uint32_t i = 0; i < MAX_SUBSCRIBERS; ++i)
    {
        if(g_subscribers[i].buffer == NULL)
            continue;
        
        if(g_subscribers[i].index >= (g_subscribers[i].buffer_nwords))
            g_subscribers[i].index = 0;
        
        if(!g_subscribers[i].index && ps2keyb_subscriber_empty(g_subscribers[i].buffer))
            continue;
        
        // put the keycode in the buffer, please
        g_subscribers[i].buffer[g_subscribers[i].index++] = keycode;        
    }
}

static uint16_t ps2keyb_get_keycode(uint16_t *set, uint8_t c, uint16_t offset)
{
    uint16_t released_offset = (uint16_t)(SCANCODE_RELEASED + offset);

    if(c >= released_offset)
        return set[c - SCANCODE_RELEASED] | KEYCODE_FLAG_KEY_RELEASED;
    
    return set[c];
}

static uint16_t ps2keyb_check_pause(uint8_t c)
{
    if(not_in(pause_key_seq, sizeof(pause_key_seq), c))
        return 0;
    
    return KEYCODE_PAUSEBREAK;
}

static uint8_t ps2keyb_idle_state_checks(uint8_t c)
{
    if(c == SCANCODE_EXTENDED_SET)
    {
        g_state = STATE_WAIT_FOR_NEXT_SCAN_CODE;
        g_flags |= FLAG_KEY_EXTENDED;
        return 1;
    }

    if(c == SCANCODE_PAUSE_START)
    {
        g_flags |= (FLAG_CHECK_FOR_PAUSE);
        g_state = STATE_WAIT_FOR_NEXT_SCAN_CODE;
        
        return sizeof(pause_key_seq);
    }

    return 0;
}

void ps2keyb_manager(uint8_t c)
{
    static uint8_t expected_scancodes = 0;
    uint16_t keycode = 0;

    switch(g_state)
    {
        case STATE_INIT:
            // No longer in use
            if(c == RESP_SELF_TEST_PASSED)
                g_flags |= FLAG_CMD_COMPLETE;
            else if(c == RESP_ACK)
                g_flags |= FLAG_ACK;
        break;

        case STATE_WAIT_FOR_RESPONSE:
            if(c == RESP_ACK)
                { g_flags |= FLAG_ACK; break; }
            
            g_response = c;
            g_flags |= FLAG_RESP_RECEIVED;
        break;

        case STATE_IDLE:
            expected_scancodes = ps2keyb_idle_state_checks(c);

            if(expected_scancodes)
                break;

            keycode = ps2keyb_get_keycode(scancode_normal, c, 0);
            ps2keyb_send_keycode(keycode);
        break;

        case STATE_WAIT_FOR_NEXT_SCAN_CODE:
            expected_scancodes--; 
            
            if(g_flags & (FLAG_KEY_EXTENDED))
                keycode = ps2keyb_get_keycode(scancode_extended1, c, SCANCDOE_EXTENDED_OFFSET);
            
            if(g_flags & (FLAG_CHECK_FOR_PAUSE))
                keycode = ps2keyb_check_pause(c);
            
            // if by this time we're still not able to give an indication of what the keycode should be
            // we'd better stop
            if(!keycode)
                expected_scancodes = 0;
            
            if(expected_scancodes)
                break;
            
            g_flags &= (uint16_t) ~(FLAG_KEY_RELEASED | FLAG_KEY_EXTENDED | FLAG_CHECK_FOR_PAUSE);
            g_state = STATE_IDLE;
            ps2keyb_send_keycode(keycode);
        break;

    }
}

void ps2keyb_isr21(void)
{
    __asm__ __volatile__ ("cli");
    uint8_t c = inb(KEYB_PORT);
    ps2keyb_manager(c);
    __asm__ __volatile__ ("sti");
}

static err_t ps2keyb_reg_subscriber(uint16_t *bfr, size_t size)
{
    // check buffer not in kernel space or size lower than the absolute minimum
    if((uint32_t)bfr < (2 * 1024 * 1024) || size < sizeof(uint16_t))
        return EXIT_CODE_GLOBAL_INVALID;
    
    size = size / sizeof(uint16_t);
    
    for(uint32_t i = 0; i < MAX_SUBSCRIBERS; ++i)
    {
        if(g_subscribers[i].buffer == NULL)
        {
            g_subscribers[i].buffer = bfr;
            g_subscribers[i].buffer_nwords = size;
            g_subscribers[i].index = 0;
            return EXIT_CODE_GLOBAL_SUCCESS;
        }
    }

    return EXIT_CODE_GLOBAL_GENERAL_FAIL;
}

static err_t ps2keyb_dereg_subscriber(uint16_t *bfr, size_t size)
{
    // check buffer not in kernel space or size lower than the absolute minimum
    if((uint32_t)bfr < (2 * 1024 * 1024) || size < sizeof(uint16_t))
        return EXIT_CODE_GLOBAL_INVALID;

    size = size / sizeof(uint16_t);

    for(uint32_t i = 0; i < MAX_SUBSCRIBERS; ++i)
    {
        if(g_subscribers[i].buffer == bfr && g_subscribers[i].buffer_nwords == size)
        {
            g_subscribers[i].buffer = NULL;
            return EXIT_CODE_GLOBAL_SUCCESS;
        }
    }

    return EXIT_CODE_GLOBAL_GENERAL_FAIL;
}

void ps2keyb_api_handler(void *req)
{
    ps2keyb_api_req *r = req;
    r->hdr.exit_code = EXIT_CODE_GLOBAL_SUCCESS;

    switch((r->hdr.system_call) & 0xFF)
    {
        case PS2KEYB_CALL_REGISTER_SUBSCRIBER:
            r->hdr.exit_code = ps2keyb_reg_subscriber(r->buffer, r->buffer_size);
        break;

        case PS2KEYB_CALL_DEREGISTER_SUBSCRIBER:
            r->hdr.exit_code = ps2keyb_dereg_subscriber(r->buffer, r->buffer_size);
        break;

        case PS2KEYB_CALL_LAST_KEY:
            r->hdr.response = g_last_key;
        break;

        default:
            r->hdr.exit_code = EXIT_CODE_GLOBAL_UNSUPPORTED;
        break;
    }
}

void ps2keyb_wait(void)
{
    while(1)
    {
        uint8_t stat = inb(COMMAND_PORT);

        if(stat & COMMAND_PORT_OBF)
            inb(KEYB_PORT);
        else if(!(stat & COMMAND_PORT_READY))
            break;
    }
}


err_t main(void)
{
    g_state = STATE_INIT;
    g_response = 0;

    kernel_add_interrupt_handler((function_t) ps2keyb_isr21, KEYB_INT);

    api_space_t space = api_get_api_space((function_t) ps2keyb_api_handler);

    if(space == (api_space_t) MAX)
        return EXIT_CODE_GLOBAL_GENERAL_FAIL;
    
    g_subscribers = valloc(SUBSCRIBER_STRUCT_SPACE);

    if(!g_subscribers)
        return EXIT_CODE_GLOBAL_OUT_OF_MEMORY;
    
    memset(g_subscribers, SUBSCRIBER_STRUCT_SPACE, 0);

    ps2keyb_wait();
    g_state = STATE_IDLE;

    // only for testing (during testing this driver is launched
    // as a program):
    program_terminate_stay_resident();
    
    return EXIT_CODE_GLOBAL_SUCCESS;
}
