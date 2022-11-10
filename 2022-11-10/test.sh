echo '/ping' | timeout 1 nc localhost 9998
echo '/reset' | timeout 1 nc localhost 9998
(timeout 10 nc localhost 9999 &)
(timeout 10 nc localhost 9999 &)
(timeout 10 nc localhost 9999 &)
echo '/clients' | timeout 1 nc localhost 9998
echo 'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA' | timeout 1 nc localhost 9999
echo 'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA' | timeout 1 nc localhost 9999
echo 'AAAAAAAAAAAAAAAAAAAAAAAAAAAAAA' | timeout 1 nc localhost 9999
sleep 1
echo '/report' | timeout 1 nc localhost 9998
