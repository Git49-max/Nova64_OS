#include <stellar/stellar.h>
#include <stdlib.h> // Para atof

int stellar_compile(char* source, uint8_t* binary_out) {
    int src_ptr = 0;
    int bin_ptr = 0;
    uint8_t pending_op = 0;
    int var_to_store = -1;

    while (source[src_ptr] != '\0') {
        char c = source[src_ptr];

        if (c == '\n' || c == '\r' || c == ';') {
            if (var_to_store != -1) {
                binary_out[bin_ptr++] = OP_STORE;
                binary_out[bin_ptr++] = var_to_store;
                var_to_store = -1;
            }
            src_ptr++; continue;
        }

        if (c <= ' ' || c == '(' || c == ',') { src_ptr++; continue; }

        // Suporte a números decimais (ex: 17.0, 3.14)
        if ((c >= '0' && c <= '9') || c == '.') {
            char num_buf[32];
            int n_ptr = 0;
            while ((source[src_ptr] >= '0' && source[src_ptr] <= '9') || source[src_ptr] == '.') {
                num_buf[n_ptr++] = source[src_ptr++];
            }
            num_buf[n_ptr] = '\0';

            binary_out[bin_ptr++] = OP_PUSH;
            double val = atof(num_buf);
            uint8_t* p = (uint8_t*)&val;
            for (int i = 0; i < 8; i++) binary_out[bin_ptr++] = p[i];

            if (pending_op) { binary_out[bin_ptr++] = pending_op; pending_op = 0; }
            continue;
        }

        if (c >= 'a' && c <= 'z') {
            int var_idx = c - 'a';
            src_ptr++;
            int temp_ptr = src_ptr;
            while(source[temp_ptr] == ' ') temp_ptr++;
            
            if (source[temp_ptr] == '=') {
                var_to_store = var_idx;
                src_ptr = temp_ptr + 1;
            } else {
                binary_out[bin_ptr++] = OP_LOAD;
                binary_out[bin_ptr++] = var_idx;
                if (pending_op) { binary_out[bin_ptr++] = pending_op; pending_op = 0; }
            }
            continue;
        }

        if (c == '+') pending_op = OP_ADD;
        else if (c == '-') pending_op = OP_SUB;
        else if (c == '*') pending_op = OP_MUL;
        else if (c == '/') pending_op = OP_DIV;
        else if (c == ')') binary_out[bin_ptr++] = OP_PRINT;

        src_ptr++;
    }

    if (var_to_store != -1) {
        binary_out[bin_ptr++] = OP_STORE;
        binary_out[bin_ptr++] = var_to_store;
    }

    binary_out[bin_ptr++] = OP_HALT;
    return bin_ptr;
}