package box

import (
	"anki/ipc"
	"anki/log"
	"context"
)

// Run starts the box service
func Run(ctx context.Context) {
	runServer(ctx)
}

func runServer(ctx context.Context) {
	serv, err := ipc.NewUnixgramServer(ipc.GetSocketPath("box_server"))
	if err != nil {
		log.Println("Error creating box server:", err)
		return
	}

	if done := ctx.Done(); done != nil {
		go func() {
			<-done
			serv.Close()
		}()
	}

	log.Println("Elemental box server is running")

	for c := range serv.NewConns() {
		cl := client{Conn: c}
		go cl.handleConn(ctx)
	}
}
