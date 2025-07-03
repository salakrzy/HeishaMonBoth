import hashlib
import os
Import('env')


OUTPUT_DIR = "build_output{}".format(os.path.sep)

def md5_generator(source, target, env):
    variant = str(target[0]).split(os.path.sep)[1]

# create string with location and file names based on variant
    file_write = "{}firmware{}{}.MD5".format(OUTPUT_DIR, os.path.sep, variant)
    file_read = "{}firmware{}{}.bin".format(OUTPUT_DIR, os.path.sep, variant)

    with open(file_read, "rb") as f:
        file_hash = hashlib.md5()
        while chunk := f.read(8192):
            file_hash.update(chunk)  
        f.close()
    print("MD5 checksum",file_hash.hexdigest())
    with open(file_write, 'w') as fw: 
        fw.write('MD5 checksum for file '+file_read+'\n') 
        fw.write(file_hash.hexdigest()) 
    fw.close()

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", [md5_generator])
