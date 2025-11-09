#include "morse_translation.h"
#include <string.h>
#include <ctype.h>

// Table mapping characters <-> Morse code strings
typedef struct {
    char        letter;
    const char *morse;
} morse_map_t;

static const morse_map_t morse_table[] = {
    // Letters
    { 'A', ".-"    }, { 'B', "-..." }, { 'C', "-.-." }, { 'D', "-.."  },
    { 'E', "."     }, { 'F', "..-." }, { 'G', "--."  }, { 'H', "...." },
    { 'I', ".."    }, { 'J', ".---" }, { 'K', "-.-"  }, { 'L', ".-.." },
    { 'M', "--"    }, { 'N', "-."   }, { 'O', "---"  }, { 'P', ".--." },
    { 'Q', "--.-"  }, { 'R', ".-."  }, { 'S', "..."  }, { 'T', "-"    },
    { 'U', "..-"   }, { 'V', "...-" }, { 'W', ".--"  }, { 'X', "-..-" },
    { 'Y', "-.--"  }, { 'Z', "--.." },

    // Digits
    { '0', "-----" }, { '1', ".----" }, { '2', "..---" }, { '3', "...--" },
    { '4', "....-" }, { '5', "....." }, { '6', "-...." }, { '7', "--..." },
    { '8', "---.." }, { '9', "----." },

    // Some extras if you want them
    { '.', ".-.-.-" }, { ',', "--..--" }, { '?', "..--.." },
};

static const size_t MORSE_TABLE_LEN = sizeof(morse_table) / sizeof(morse_table[0]);

// mapping helpers 

const char *char_to_morse(char ch)
{
    ch = (char)toupper((unsigned char)ch);

    for (size_t i = 0; i < MORSE_TABLE_LEN; i++) {
        if (morse_table[i].letter == ch) {
            return morse_table[i].morse;
        }
    }
    // normal space is handled in text_to_morse(), not here
    return NULL;
}

char morse_to_char(const char *code)
{
    if (code == NULL || *code == '\0') {
        return '?';
    }

    for (size_t i = 0; i < MORSE_TABLE_LEN; i++) {
        if (strcmp(morse_table[i].morse, code) == 0) {
            return morse_table[i].letter;
        }
    }
    return '?'; // unknown pattern
}

// --- TEXT -> MORSE -----------------------------------------------------------
//
// Rules:
// - 1 space between letters
// - 2 spaces between words
// - 3 spaces at end of message

void text_to_morse(const char *text, char *out, size_t out_len)
{
    if (out_len == 0) {
        return;
    }
    out[0] = '\0';

    if (text == NULL) {
        return;
    }

    size_t pos = 0;
    int wrote_any        = 0;   // have we output at least one symbol?
    int pending_word_gap = 0;   // did we just see a space in the input text?

    for (const char *p = text; *p != '\0'; ++p) {
        char ch = *p;

        if (ch == ' ') {
            // the next non-space letter should start a new word
            if (wrote_any) {
                pending_word_gap = 1;
            }
            continue;
        }

        const char *morse = char_to_morse(ch);
        if (!morse) {
            // unsupported character, skip it
            continue;
        }

        // insert separators according to protocol
        if (wrote_any) {
            if (pending_word_gap) {
                // 2 spaces between words
                if (pos + 2 >= out_len) break;
                out[pos++] = ' ';
                out[pos++] = ' ';
            } else {
                // 1 space between letters
                if (pos + 1 >= out_len) break;
                out[pos++] = ' ';
            }
        }

        pending_word_gap = 0;

        size_t mlen = strlen(morse);
        if (pos + mlen >= out_len) {
            // no space for full symbol, stop
            break;
        }
        memcpy(&out[pos], morse, mlen);
        pos += mlen;
        out[pos] = '\0';

        wrote_any = 1;
    }

    // append end-of-message terminator: 3 spaces
    if (wrote_any) {
        if (pos + 3 < out_len) {
            out[pos++] = ' ';
            out[pos++] = ' ';
            out[pos++] = ' ';
            out[pos]   = '\0';
        } else if (pos < out_len) {
            // at least terminate
            out[pos] = '\0';
        }
    }
}

// --- MORSE -> TEXT -----------------------------------------------------------
//
// Input is something like:
//   ".- -...  -.-.   "
// Output becomes:
//   "AB C"

void morse_to_text(const char *morse, char *out, size_t out_len)
{
    if (out_len == 0) {
        return;
    }
    out[0] = '\0';

    if (morse == NULL) {
        return;
    }

    size_t pos = 0;             // index in output text
    char   buf[16];             // current letter's Morse pattern
    size_t buf_len   = 0;
    int    space_cnt = 0;

    for (size_t i = 0; morse[i] != '\0'; ++i) {
        char c = morse[i];

        if (c == '.' || c == '-') {
            // part of a Morse symbol
            if (buf_len + 1 < sizeof(buf)) {
                buf[buf_len++] = c;
                buf[buf_len]   = '\0';
            }
            space_cnt = 0;  // reset after any symbol
        } else if (c == ' ') {
            space_cnt++;

            if (space_cnt == 1) {
                // end of letter: decode buffer
                if (buf_len > 0) {
                    char letter = morse_to_char(buf);
                    if (pos + 1 < out_len) {
                        out[pos++] = letter;
                        out[pos]   = '\0';
                    }
                    buf_len   = 0;
                    buf[0]    = '\0';
                }
            } else if (space_cnt == 2) {
                // word separator
                if (pos + 1 < out_len) {
                    out[pos++] = ' ';
                    out[pos]   = '\0';
                }
            } else if (space_cnt >= 3) {
                // end of message
                break;
            }
        } else {
            // ignore unknown characters
        }
    }

    // flush last letter if message didn't properly end with spaces
    if (buf_len > 0 && pos + 1 < out_len) {
        char letter = morse_to_char(buf);
        out[pos++] = letter;
        out[pos]   = '\0';
    }
}
