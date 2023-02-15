/*
MIT license
Copyright (c) 2023 Maarten Vermeulen

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

#include "api.h"
#include "util.h"

#include "include/cp_api.h"
#include "include/cp.h"
#include "include/cp_exit_codes.h"
#include "include/commands.h"
#include "include/fileman.h"
#include "include/processor.h"

api_space_t g_api_space = 0;

void cp_api_set_space(api_space_t s)
{
    g_api_space = s;
}

void cp_api_handler(void *r)
{
    cp_api_req *req = r;
    req->hdr.exit_code = EXIT_CODE_GLOBAL_SUCCESS;

    switch(req->hdr.system_call % g_api_space)
    {
        case CP_GET_VERSION:
        {
            char *str = command_create_cp_ver_str();
            req->hdr.response_ptr = str;
            req->hdr.response_size = strlen(str);

            break;
        }

        case CP_GET_CWD:
            req->hdr.response_ptr = valloc(MAX_PATH_LEN);
            getcwd(req->hdr.response_ptr, &req->hdr.response_size);
        break;

        case CP_SET_CWD:
            req->hdr.exit_code = setcwd(req->param);
        break; 

        case CP_EXEC_CMD:
        {
            size_t len = strlen(req->param);
            char *original = valloc(len), *uc = valloc(len);
            
            // processor modifies the command buffer, so we need to copy it here
            // to not mess up the original given by the caller
            memcpy(original, req->param, len + 1);
            memcpy(uc, req->param, len + 1);

            // all commands are checked uppercase
            to_uc(uc, len);

            processor_execute_command(uc, original);
            vfree(uc);
            vfree(original);
            break;   
        }   

        case CP_GET_ERRLVL:
            req->hdr.response = (uint32_t) processor_get_last_error();
        break;

        default:
            req->hdr.exit_code = EXIT_CODE_GLOBAL_UNSUPPORTED;
        break;
    }
}

