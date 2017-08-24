./client -l $1 -c pippo &
./client -l $1 -c pluto &
./client -l $1 -c minni &
./client -l $1 -c topolino &
./client -l $1 -c paperino &
./client -l $1 -c qui &
echo "OK"
wait
echo "OK"
./client -l $1 -k pippo -g gruppo1
echo "OK"
./client -l $1 -k pluto -g gruppo2
echo "OK"
./client -l $1 -k minni -a gruppo1
echo "OK"
./client -l $1 -k topolino -a gruppo1
echo "OK"
./client -l $1 -k paperino -a gruppo2
echo "OK"
./client -l $1 -k minni -a gruppo2
echo "OK"
./client -l $1 -k pippo -p
echo "OK"
./client -l $1 -k pluto -p
echo "OK"
./client -l $1 -k minni -p
echo "OK"
./client -l $1 -k topolino -p
echo "OK"
./client -l $1 -k topolino -d gruppo1
echo "OK"
./client -l $1 -k minni -d gruppo2
echo "OK"

