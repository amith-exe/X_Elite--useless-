#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bmp.h" // We still need your bmp.h definitions

// A unique marker to signify the end of our hidden message.
const char *EOM_MARKER = "$!EOM!$";

// This function will be called from Python to hide a message.
// It takes raw image data in memory and returns the modified data.
int encode(const BYTE* input_pixel_data, int data_size, int height, int width, const char *message, BYTE** output_pixel_data)
{
    // Allocate memory for the output image data and copy the original pixels into it
    *output_pixel_data = (BYTE*)malloc(data_size);
    if (*output_pixel_data == NULL) return 1; // Memory allocation failed
    memcpy(*output_pixel_data, input_pixel_data, data_size);

    // Create a 2D array "view" of the 1D pixel data for easier access
    RGBTRIPLE (*image)[width] = (RGBTRIPLE(*)[width])(*output_pixel_data);

    // --- This is the proven encoding logic from your helper.c ---
    int message_len = strlen(message);
    int eom_len = strlen(EOM_MARKER);
    int total_len = message_len + eom_len;
    long bits_to_hide = total_len * 8;
    long max_bits = height * width * 3;

    if (bits_to_hide > max_bits)
    {
        free(*output_pixel_data); // Clean up memory on failure
        return 1; // Message too long
    }

    long bit_index = 0;
    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            BYTE *channels[] = {&image[i][j].rgbtBlue, &image[i][j].rgbtGreen, &image[i][j].rgbtRed};
            for (int c = 0; c < 3; c++)
            {
                if (bit_index < bits_to_hide)
                {
                    int char_index = bit_index / 8;
                    int bit_pos = bit_index % 8;
                    char current_char = (char_index < message_len) ? message[char_index] : EOM_MARKER[char_index - message_len];
                    int message_bit = (current_char >> bit_pos) & 1;

                    if (message_bit == 0)
                    {
                        *channels[c] &= 0xFE; // Set LSB to 0
                    }
                    else
                    {
                        *channels[c] |= 0x01; // Set LSB to 1
                    }
                    bit_index++;
                }
                else
                {
                    return 0; // Success
                }
            }
        }
    }
    return 0; // Success
}

// This function will be called from Python to reveal a message.
void decode(const BYTE* pixel_data, int height, int width, char* revealed_message_buffer)
{
    RGBTRIPLE (*image)[width] = (RGBTRIPLE(*)[width])(pixel_data);
    // --- This is the proven decoding logic from your helper.c ---
    char revealed_message[height * width * 3 / 8 + 1];
    int msg_index = 0;
    char current_byte = 0;
    int bit_count = 0;

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            BYTE *channels[] = {&image[i][j].rgbtBlue, &image[i][j].rgbtGreen, &image[i][j].rgbtRed};
            for (int c = 0; c < 3; c++)
            {
                int lsb = *channels[c] & 1;
                current_byte |= (lsb << bit_count);
                bit_count++;

                if (bit_count == 8)
                {
                    revealed_message[msg_index++] = current_byte;
                    revealed_message[msg_index] = '\0';

                    if (strstr(revealed_message, EOM_MARKER) != NULL)
                    {
                        revealed_message[msg_index - strlen(EOM_MARKER)] = '\0';
                        strcpy(revealed_message_buffer, revealed_message);
                        return;
                    }
                    bit_count = 0;
                    current_byte = 0;
                }
            }
        }
    }
    strcpy(revealed_message_buffer, "Error: No hidden message found.");
}

// We need a function that Python can call to free the memory that 'encode' allocated
void free_memory(void* ptr) {
    free(ptr);
}