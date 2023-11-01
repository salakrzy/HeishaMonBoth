Import("env")
env.Replace( MKFSTOOL=env.get("PROJECT_DIR") + '/piotools/mklittlefs' )  # PlatformIO now believes it has actually created a SPIFFS
