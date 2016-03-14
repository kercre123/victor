import os
import sys

def main(src = '', dst = ''):
    files = os.listdir(src)
    for file in files:
        newname = file.replace('side1', 'front').replace('side2', 'right').replace('side3', 'back').replace('side4','left')
        print('Old: ' + file + ' -> New: ' + newname)
        os.rename(os.path.join(src, file), os.path.join(dst, newname))
        
    if not os.path.exists(dst):
        os.mkdir(dst)

if(len(sys.argv)<2):
    print('Need at least one arg: source dir')      
elif(len(sys.argv)<3):
    main(str(sys.argv[1]), str(sys.argv[1]))
else:
    main(str(sys.argv[1]), str(sys.argv[2]))
    

