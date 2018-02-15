#!/usr/bin/env python

import os
import re


to_remove = ['lib/*', 'build/*']
to_remove_pattern = ['.*cmake.*', 'makefile', '.*\.a$', '.*\.so', 'a.out']
exceptions = ['CMakeLists.txt']

cwd = os.getcwd()


def should_be_removed(path):
    path = os.path.basename(path)

    for pattern in to_remove_pattern:
        if re.match(pattern, path, re.IGNORECASE):
            for exception in exceptions:
                if exception == path:
                    return False
            return True
    return False


def remove(path):
    os.system('rm -rf %s' % path)


for item in to_remove:
    path = os.path.join(cwd, item)
    remove(path)


for root, dirs, files in os.walk(cwd):
    for dname in dirs:
        path = os.path.join(root, dname)
        if should_be_removed(path):
            remove(path)
    for fname in files:
        path = os.path.join(root, fname)
        if should_be_removed(path):
            remove(path)
