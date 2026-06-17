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
def havekanji(text):
    for char in text:
        if '\u4E00' <= char <= '\u9FFF' or '\u3400' <= char <= '\u4DBF':
            return True
    return False
files = getfiles('.')
kanjifiles = []
common = "`1234567890-=qwertyuiop[]\\asdfghjkl;'zxcvbnm,./~!@#$%^&*()_+QWERTYUIOP{}|ASDFGHJKL:\"ZXCVBNM<>? \n\r\t\b\f©"
for i in files:
    if i.startswith('.\\.git\\'):
        continue
    else:
        try:
            f = open(i, 'r').read()
            if havekanji(f):
                kanjifiles.append(i)
        except:
            continue
with open('.\\.github\\workflows\\kanjifiles.txt', 'w') as f:
    for i in kanjifiles:
        print(i, file = f)
'''
for i in files:
    try:
        f = open('..\\%s' % k, 'r').read()
    except:
        continue
'''