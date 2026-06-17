import os
def getfiles(path):
    e = []
    try:
        f = os.listdir(path)
    except:
        return path
    for i in f:
        d = getfiles('%s\\%s' % (path, i))
        try:
            e = e + d
        except:
            e.append(d)
    return e
files = getfiles('..\\NetHack-cn')
kanjifiles = []
common = "`1234567890-=qwertyuiop[]\\asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:\"ZXCVBNM<>? \n\r\t\b\f"
for i in files:
	try:
		f = open(i, 'r').read()
		for j in f:
			if not j in common:
				kanjifiles.append(i)
				break
	except:
		continue
print(kanjifiles)
'''
for i in files:
    try:
        f = open('..\\%s' % k, 'r').read()
    except:
        continue
'''