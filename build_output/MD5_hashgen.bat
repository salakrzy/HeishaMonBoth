echo  to use run    MD5_hasgen.bat filename
cmd /c CertUtil -hashfile %1 MD5 >> %1".md5"
