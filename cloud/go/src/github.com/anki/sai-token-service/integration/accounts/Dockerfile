from anki/sai-go-accounts:develop

run DB_TYPE=sqlite /sai-go-accounts create-tables

run DB_TYPE=sqlite /sai-go-accounts create-app-key -token=user -scopemask=user -expiry="2035-01-01T00:00:00"
run DB_TYPE=sqlite /sai-go-accounts create-app-key -token=admin -scopemask=admin -expiry="2035-01-01T00:00:00"
run DB_TYPE=sqlite /sai-go-accounts create-app-key -token=support -scopemask=support -expiry="2035-01-01T00:00:00"
run DB_TYPE=sqlite /sai-go-accounts create-app-key -token=ankival -scopemask=service -expiry="2035-01-01T00:00:00"
run DB_TYPE=sqlite /sai-go-accounts create-app-key -token=tokenservice -scopemask=service -expiry="2035-01-01T00:00:00"
run DB_TYPE=sqlite /sai-go-accounts create-app-key -token=jdocservice -scopemask=service -expiry="2035-01-01T00:00:00"
run DB_TYPE=sqlite /sai-go-accounts create-session -ownerid=anki@example.com -scope=admin -sessionid=admin -expiry="2035-01-01T00:00:00"
run DB_TYPE=sqlite /sai-go-accounts create-session -ownerid=anki@example.com -scope=user -sessionid=user -expiry="2035-01-01T00:00:00"
run DB_TYPE=sqlite /sai-go-accounts create-session -ownerid=anki@example.com -scope=support -sessionid=support -expiry="2035-01-01T00:00:00"
run DB_TYPE=sqlite /sai-go-accounts create-session -ownerid=anki@example.com -scope=service -sessionid=service -expiry="2035-01-01T00:00:00"
run DB_TYPE=sqlite /sai-go-accounts create-session -ownerid=anki@example.com -scope=service -sessionid=tokenservice -expiry="2035-01-01T00:00:00"
run DB_TYPE=sqlite /sai-go-accounts create-session -ownerid=anki@example.com -scope=service -sessionid=jdocservice -expiry="2035-01-01T00:00:00"

cmd [ "/sai-go-accounts", "start" ]
