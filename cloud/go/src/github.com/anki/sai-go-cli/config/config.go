package config

import (
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"time"

	"github.com/gwatts/iniconf"
)

const (
	ConfigFilename     = ".saiconfig"
	EnvConfig          = "SAI_CLI_CONFIG"
	EnvHome            = "HOME"
	DefaultSessionName = "default"
)

var (
	UnknownEnvironment = errors.New("Unknown environment name")
	ConfigNotFound     = errors.New("No configuration file specified")
	ErrUnknownSession  = errors.New("Invalid session name for environment")
)

type Service string

const (
	Accounts            Service = "accounts"
	Ankival             Service = "ankival"
	Audit               Service = "audit"
	Blobstore           Service = "blobstore"
	VirtualRewards      Service = "virtualrewards"
	VirtualRewardsQueue Service = "virtualrewards_queue"
	RedshiftLoaderQueue Service = "redshift_loader"
	DAS                 Service = "das"
	Jdocs               Service = "jdocs"
)

type ServiceURLs struct {
	AccountsURL            string `iniconf:"accounts_url"`
	AnkivalURL             string `iniconf:"ankival_url"`
	AuditURL               string `iniconf:"audit_url"`
	BlobstoreURL           string `iniconf:"blobstore_url"`
	VirtualRewardsURL      string `iniconf:"virtualrewards_url"`
	VirtualRewardsQueueURL string `iniconf:"virtualrewards_queue_url"`
	RedshiftLoaderQueueURL string `iniconf:"redshift_loader_queue_url"`
	DASQueueURL            string `iniconf:"das_queue_url"`
	JdocsURL               string `iniconf:"jdocs_url"`
}

func (s *ServiceURLs) ByService(srv Service) string {
	switch srv {
	case Accounts:
		return s.AccountsURL
	case Ankival:
		return s.AnkivalURL
	case Audit:
		return s.AuditURL
	case Blobstore:
		return s.BlobstoreURL
	case VirtualRewards:
		return s.VirtualRewardsURL
	case VirtualRewardsQueue:
		return s.VirtualRewardsQueueURL
	case RedshiftLoaderQueue:
		return s.RedshiftLoaderQueueURL
	case DAS:
		return s.DASQueueURL
	case Jdocs:
		return s.JdocsURL
	default:
		return ""
	}
}

type Environment struct {
	EnvName     string
	AdminKey    string `iniconf:"admin_key"`
	UserKey     string `iniconf:"user_key"`
	SupportKey  string `iniconf:"support_key"`
	CronKey     string `iniconf:"cron_key"`
	ServiceKey  string `iniconf:"service_key"`
	ServiceURLs *ServiceURLs
	c           *Config
}

func (e Environment) AppKeyByName(name string) (string, error) {
	switch name {
	case "admin":
		return e.AdminKey, nil
	case "user":
		return e.UserKey, nil
	case "support":
		return e.SupportKey, nil
	case "cron":
		return e.CronKey, nil
	case "service":
		return e.ServiceKey, nil
	default:
		return "", errors.New("Invalid app key name")
	}
}

type Session struct {
	SessionName string
	TimeCreated string `iniconf:"time_created"`
	UserID      string `iniconf:"user_id"`
	AppKey      string `iniconf:"app_key"`
	Token       string `iniconf:"token"`
	c           *Config
}

func (s *Session) Env() (*Environment, error) {
	return s.c.Env, nil
}

func (s *Session) Save() error {
	return s.c.saveSession(s)
}

func (s *Session) Delete() error {
	return s.c.deleteSession(s)
}

// Common holds global settings for the program
type Common struct {
	DumpRequest  string `iniconf:"dump_request"`
	DumpResponse string `iniconf:"dump_response"`
}

// Config handles loading, updating and saving of configuration information
// and merging persisted configuration with transient per-run configuration.
//
// On Load, Common, Env and Session are populated using the on-disk configuration
// file (if any) - The user may then overwrite those values to customize
// run-time behaviour.
type Config struct {
	Common  Common       // Data from the [common] configuration section
	Env     *Environment // Currently selected [env.envname] section
	Session *Session     // Currently selected [session.envname.session-name] section

	base  *iniconf.IniConf
	chain *iniconf.ConfChain
}

// Load loads configuration data from disk.
func Load(fn string, failOnNotFound bool, envName, sessionName string) (cfg *Config, err error) {
	if fn == "" {
		fn, err = findConfigurationFile()
		if err != nil && err != ConfigNotFound {
			return nil, err
		}
	}

	c, err := iniconf.LoadFile(fn, false)
	if err != nil {
		if os.IsNotExist(err) {
			if failOnNotFound {
				return nil, err
			}
			c = iniconf.NewFile(fn, false)
		} else {
			return nil, err
		}

	}

	defaults, err := iniconf.LoadString(defaultConfig, false)
	if err != nil {
		panic(fmt.Sprintf("Failed to parse default config: %v", err))
	}
	cfg = &Config{
		chain: iniconf.NewConfChain(c, defaults),
		base:  c,
	}
	cfg.chain.ReadSection("common", &cfg.Common)

	env, err := cfg.GetEnvironment(envName)
	if err != nil {
		return nil, err
	}
	cfg.Env = env

	var s *Session
	if sessionName != "" {
		s, err = cfg.GetSession(sessionName)
		if err != nil && sessionName != DefaultSessionName {
			return nil, err
		}
	}
	if s == nil {
		cfg.Session = &Session{
			c:           cfg,
			SessionName: sessionName,
			AppKey:      cfg.Env.UserKey,
		}
	} else {
		cfg.Session = s
	}

	return cfg, nil
}

// GetEnvironment fetches the configuration for the named environment from
// the configuration file.
func (c *Config) GetEnvironment(envName string) (*Environment, error) {
	var env Environment
	section := "env." + envName
	if !c.chain.HasSection(section) {
		return nil, UnknownEnvironment
	}
	if err := c.chain.ReadSection(section, &env); err != nil {
		return nil, err
	}
	env.c = c
	env.EnvName = envName
	return &env, nil
}

// GetSession fetches the configuration for the named session from the
// configuration file.
func (c *Config) GetSession(sessionName string) (*Session, error) {
	var s Session
	section := c.sessionSectionName(sessionName)
	if !c.chain.HasSection(section) {
		return nil, ErrUnknownSession
	}
	if err := c.chain.ReadSection(section, &s); err != nil {
		return nil, err
	}
	s.SessionName = sessionName
	s.c = c
	return &s, nil
}

// NewSession creates a new named session and sets its environment and appkey
// to the currently active values.  It does not set a sesion token and does
// not persist it to disk (use the Session.Save method to save).
func (c *Config) NewSession(sessionName string) (*Session, error) {
	s := &Session{
		SessionName: sessionName,
		TimeCreated: time.Now().Format(time.RFC3339),
		AppKey:      c.Session.AppKey,
		c:           c,
	}
	return s, nil
}

func (c *Config) saveSession(s *Session) error {
	if s.SessionName == "" {
		return errors.New("Empty session name")
	}

	if err := c.base.UpdateSection(c.sessionSectionName(s.SessionName), s); err != nil {
		return err
	}
	return c.base.Save()
}

func (c *Config) deleteSession(s *Session) error {
	if err := c.base.DeleteSection(c.sessionSectionName(s.SessionName)); err != nil {
		return err
	}
	return c.base.Save()
}

func (c *Config) sessionSectionName(name string) string {
	return "session." + c.Env.EnvName + "." + name
}

// SetActiveAppKeyName updates Session.AppKey to the AppKey defined
// in the current active environment.
func (c *Config) SetActiveAppKeyName(keyName string) error {
	key, err := c.Env.AppKeyByName(keyName)
	if err != nil {
		return err
	}
	if key == "" {
		return errors.New("No key configured for " + keyName)
	}
	c.Session.AppKey = key
	return nil
}

func (c *Config) SetDumpRequest(opt string) error {
	c.Common.DumpRequest = opt
	return nil
}

func (c *Config) SetDumpResponse(opt string) error {
	c.Common.DumpResponse = opt
	return nil
}

// findConfigurationFile searches for an returns an absolute path to
// the CLI configuration file. The path to the config file can be
// found in one of several ways, in the following order:
//
//    * If the --file command-line argument has been specified, that
//    overrides the search and will be returned.
//    * The environment variable SAI_CLI_CONFIG
//    * Search for a config file using a Node.js style algorithm,
//    walking up the directory tree starting at the current directory,
//    and terminating when we find a config file or reach /
//    * Finally, if that directory search didn't net a config file,
//    check in $HOME
func findConfigurationFile() (string, error) {
	// Check for an explict environment config
	if cfgFile := os.Getenv(EnvConfig); cfgFile != "" {
		return cfgFile, nil
	}

	// 3. Search the current directory and its parents
	cwd, err := os.Getwd()
	if err != nil {
		return "", err
	}
	for cwd != "/" {
		path, err := filepath.Abs(filepath.Join(cwd, ConfigFilename))
		if err != nil {
			return "", err
		}
		if _, err := os.Stat(path); err == nil {
			return path, nil
		} else {
			// Navigate to parent
			cwd = filepath.Clean(filepath.Dir(cwd))
		}
	}

	// 4. Check in the user's home directory
	path := filepath.Join(filepath.Clean(os.Getenv(EnvHome)), ConfigFilename)
	if _, err := os.Stat(path); err == nil {
		return path, nil
	}

	return path, ConfigNotFound
}
