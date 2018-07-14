package main

import (
	"bufio"
	"bytes"
	"encoding/binary"
	"errors"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"path"

	"github.com/anki/sai-go-accounts/client/accounts"
	"github.com/anki/sai-go-cli/apiutil"
	"github.com/anki/sai-go-cli/config"
	wav "github.com/youpy/go-wav"

	"github.com/anki/sai-blobstore/client/blobstore"
)

func main() {
	outdir := flag.String("outdir", "", "optional - output directory for wav files")
	user := flag.String("user", "", "optional - username to log in to account (must be used with -pass)")
	pass := flag.String("pass", "", "optional - password to log in to account (must be used with -user)")
	flag.Parse()
	if flag.NArg() == 0 && *user == "" && *pass == "" {
		flag.Usage()
		return
	}

	if (*user == "") != (*pass == "") {
		log.Fatalln("-user and -pass options must be used together (only one supplied)")
	}
	if *user != "" {
		if err := doLogin(*user, *pass); err != nil {
			fmt.Println("Error logging in:", err)
		}
		return
	}

	if *outdir != "" {
		if err := os.Mkdir(*outdir, os.ModePerm); err != nil && !os.IsExist(err) {
			log.Fatalln("Couldn't create output dir:", err)
		}
	}

	sessions := flag.Args()
	c := newClient()

	for _, session := range sessions {
		ids, err := getSessionIds(c, session)
		if err != nil {
			fmt.Printf("Error getting ids for session %s: %s\n", session, err)
			continue
		}

		for i, id := range ids {
			var postfix = ""
			if len(ids) > 1 {
				postfix = fmt.Sprint("-", i)
			}
			filename := fmt.Sprintf("%s%s.wav", session, postfix)
			if *outdir != "" {
				filename = path.Join(*outdir, filename)
			}
			if err := downloadAndSave(c, id, filename); err != nil {
				fmt.Println("Error downloading and processing", id, "for session", session, ":", err)
			}
		}
	}
}

func newClient() *blobstore.Client {
	cfg, err := config.Load("", true, "dev", "default")
	if err != nil {
		log.Fatalln("Error getting default config:", err,
			"\nIf you haven't logged in with an Anki account, try:",
			"\n"+os.Args[0], " -user <username> -pass <password>")

	}
	apicfg, err := apiutil.ApiClientCfg(cfg, config.Blobstore)
	if err != nil {
		log.Fatalln("Error getting API config:", err)
	}
	client, err := blobstore.New("sai-go-cli", apicfg...)
	if err != nil {
		log.Fatalln("Error getting blobstore client:", err)
	}
	return client
}

func downloadAndSave(c *blobstore.Client, id string, filename string) error {
	resp, err := c.Download("chipper-dev", id)
	if err != nil {
		return err
	} else if resp.StatusCode != http.StatusOK {
		return errors.New(fmt.Sprint("http status ", resp.StatusCode))
	}

	fmt.Printf("Downloading %s bytes...\n", resp.Header.Get("Srv-Blob-Size"))
	var buf bytes.Buffer
	n, err := io.Copy(&buf, resp.Body)
	if err != nil {
		return err
	}

	samples := make([]wav.Sample, n/2)
	for i := 0; i < len(samples); i++ {
		var sample int16
		if err = binary.Read(&buf, binary.LittleEndian, &sample); err != nil {
			return err
		}
		samples[i].Values[0] = int(sample)
	}

	f, err := os.Create(filename)
	if err != nil {
		return err
	}

	fw := bufio.NewWriter(f)
	writer := wav.NewWriter(fw, uint32(len(samples)), 1, 16000, 16)
	if err = writer.WriteSamples(samples); err != nil {
		return err
	}
	if err = fw.Flush(); err != nil {
		return err
	}
	if err = f.Sync(); err != nil {
		return err
	}
	if err = f.Close(); err != nil {
		return err
	}
	fmt.Println("Wrote", filename)
	return nil
}

func getSessionIds(c *blobstore.Client, session string) ([]string, error) {
	search := blobstore.SearchParams{
		Key:        "Usr-chipper-session",
		SearchMode: "eq",
		Value:      session,
		MaxResults: 20,
	}

	resp, err := c.Search("chipper-dev", search)
	if err != nil {
		return nil, err
	} else if resp.StatusCode != http.StatusOK {
		str := fmt.Sprint("http status ", resp.StatusCode)
		if buf, err := ioutil.ReadAll(resp.Body); err != nil && len(buf) > 0 {
			str += "(" + string(buf) + ")"
		}
		return nil, errors.New(str)
	}

	data, err := resp.Json()
	if err != nil {
		return nil, err
	}

	results, ok := data["results"].([]interface{})
	if !ok || len(results) == 0 {
		return nil, errors.New("No results")
	}

	var ret []string
	for _, result := range results {
		kvs, ok := result.(map[string]interface{})
		if !ok {
			return nil, errors.New(fmt.Sprint("Unexpected entry in results: ", result))
		}
		id, ok := kvs["id"].(string)
		if !ok {
			return nil, errors.New(fmt.Sprint("Result has no id: ", result))
		}
		ret = append(ret, id)
	}
	return ret, nil
}

func doLogin(user, pass string) error {
	cfg, err := config.Load("", false, "dev", "default")
	if err != nil {
		return err
	}
	apicfg, err := apiutil.ApiClientCfg(cfg, config.Accounts)
	if err != nil {
		return err
	}
	c, err := accounts.NewAccountsClient("sai-go-cli", apicfg...)
	if err != nil {
		return err
	}
	resp, err := c.NewUserSession(user, pass)
	if err != nil {
		return err
	} else if resp.StatusCode != http.StatusOK {
		return errors.New(fmt.Sprint("http status ", resp.StatusCode))
	}

	jresp, err := resp.Json()
	if err != nil {
		return err
	}

	s, err := cfg.NewSession("default")
	if err != nil {
		return err
	}

	if userID, _ := jresp.FieldStr("user", "user_id"); userID != "" {
		s.UserID = userID
	}

	token, err := jresp.FieldStr("session", "session_token")
	if err != nil {
		return err
	}
	s.Token = token

	if err := s.Save(); err != nil {
		return err
	}

	fmt.Println("Successfully logged in")
	return nil
}
