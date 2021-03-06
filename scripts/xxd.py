#
# This script converts resources into C++ headers such that the software can compile with.
#

#!/usr/bin/env python

import os
import sys
import subprocess

os.system('rm -rf src/resources/*')

def xxd(src, dst):
    os.system('xxd -i ' + src + ' ' + ' ' + dst)

data  = [ 'data/manuals/anaquin.txt',
          'data/manuals/RnaAlign.txt',
          'data/manuals/RnaAssembly.txt',
          'data/manuals/RnaExpression.txt',
          'data/manuals/RnaFoldChange.txt',
          'data/manuals/RnaSubsample.txt',

          'src/r/plotFold.R',
          'src/r/plotTROC.R',
          'src/r/plotTLODR.R',
          'src/r/plotLinear.R',
          'src/r/plotConjoint.R',
    	  'src/r/plotLogistic.R',
        ]
tests = [ ]

for file in os.listdir('data/manuals/'):
    path = os.path.join('data/manuals/', file)
    if os.path.isfile(path):
        os.system('expand -t 4 ' + path + ' > /tmp/tmp.txt')
        os.system('mv /tmp/tmp.txt ' + path)

r = data
for i in range(0,len(r)):
    file = os.path.basename(r[i])
    xxd(r[i], 'src/resources/' + file)
    
    for file in os.listdir('src/resources/'):
        path = os.path.join('src/resources/', file)
        if os.path.isfile(path):
            with open(path, 'r') as f:
                data = f.read()
                if ('0x0a, 0x0a\n' in data):
                    raise Exception('Failed to embed: ' + path)
                    
os.system('rm src/data/resources.o')
