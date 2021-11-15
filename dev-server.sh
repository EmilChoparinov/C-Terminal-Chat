make clean; make && sudo fuser -k 4000/tcp
read -p "Start local server with debugger on port 4000 [Y/n]? " yn
case $yn in
    [Yy]* ) clear && ./server 4000 DEBUG;;
    *) echo "Closing";;
esac