echo  to use run    hasgen.bat filename
cmd /c CertUtil -hashfile %1 MD5 >> %1".md5"
