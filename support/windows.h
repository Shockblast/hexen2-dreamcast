#ifndef	_FAKE_WINDOWS_H_
#define	_FAKE_WINDOWS_H_

typedef	unsigned int UINT;
#define	DeleteFile(name)	unlink(name)
extern int GetTempPath(size_t size,char* buf);

#endif
