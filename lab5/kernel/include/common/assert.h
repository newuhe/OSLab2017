#ifndef __ASSERT_H__
#define __ASSERT_H__

int abort(const char *, int);
void putChar(char);

/* assert: 断言条件为真，若为假则蓝屏退出 */
#define assert(cond) \
	((cond) ? (0) : (abort(__FILE__, __LINE__)))

static inline void Log(char *msg) {
	putChar('-');putChar('-');putChar('-');putChar('-');putChar('>');putChar(' ');
	while (msg != 0 && *msg != '\0') {
		putChar(*msg++);
	}
	putChar(' ');putChar('<');putChar('-');putChar('-');putChar('-');putChar('-');
	putChar('\n');
}

static inline void panic(char *msg) {
	Log(msg);
	assert(0);
}


#endif
