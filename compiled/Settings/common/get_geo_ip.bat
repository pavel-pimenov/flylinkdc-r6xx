rem wget -N --tries=999 http://upd.emule-security.org/ipfilter.zip
rem 7z.exe x ipfilter.zip -y
rem del ipfilter.zip
rem del P2PGuard.ini
rem ren guarding.p2p P2PGuard.ini
rem call p2pguard_convert_build.bat
rem p2pguard_convert.exe
rem del P2PGuard.ini
rem ren P2PGuard_flylink.ini P2PGuard.ini
rem copy P2PGuard.ini ..
