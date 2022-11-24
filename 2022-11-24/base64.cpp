/*
 * Lab problem set for INP course
 * by Chun-Ying Huang <chuang@cs.nctu.edu.tw>
 * License: GPLv2
 */

static inline int b64decode_code(int ch);

/*
static unsigned char *b64encode(unsigned char *in, int inlen, unsigned char *output, int *outlen) {
	static char t[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	unsigned char ch = 0, *optr = output;
	int i, pad = 0;

	if (output == 0) return output;
	if (inlen <= 0) { *output = 0; return output; }
	for (i = 0; i < inlen; i++) {
		switch(i % 3) {
		case 0:
			*optr++ = t[in[i]>>2];
			ch = (in[i] & 0x3)<<4;
			pad = 2;
			break;
		case 1:
			*optr++ = t[ch | (in[i]>>4)];
			ch = (in[i] & 0xf)<<2;
			pad = 1;
			break;
		case 2:
			*optr++ = t[ch | (in[i]>>6)];
			*optr++ = t[in[i] & 0x3f];
			pad = 0;
			break;
		}
	}
	if (pad > 0) *optr++ = t[ch];
	while(pad-- > 0) *optr++ = '=';
	*optr = '\0';
	*outlen = optr - output;
	return output;
}
*/

static unsigned char *b64decode(unsigned char *in, int inlen, unsigned char *output, int *outlen) {
	unsigned char ch = 0, *optr = output;
	int i, code, coff = 0, flush = 1;

	if (output == 0) return output;
	if (inlen <= 0) { *output = 0; return output; }

	for (i = 0; i < inlen; i++) {
		if (in[i] == '=') { break; }
		if ((code = b64decode_code(in[i])) < 0) continue;
		switch (coff & 0x3) {
		case 0:
			ch = code<<2;
			flush = 1;
			break;
		case 1:
			*optr++ = ch | (code>>4);
			ch = (code & 0xf)<<4;
			flush = 0;
			break;
		case 2:
			*optr++ = ch | (code>>2);
			ch = (code & 0x3)<<6;
			break;
		case 3:
			*optr++ = ch | code;
			ch = 0;
			break;
		}
		coff++;
	}
	if (flush) *optr++ = ch;
	*optr = '\0';
	*outlen = optr - output;
	return output;
}

static inline int b64decode_code(int ch) {
	if (ch >= 'A' && ch <= 'Z') return ch - 'A';
	if (ch >= 'a' && ch <= 'z') return 26 + ch - 'a';
	if (ch >= '0' && ch <= '9') return 52 + ch - '0';
	if (ch == '+') return 62;
	if (ch == '/') return 63;
	return -1;
}
