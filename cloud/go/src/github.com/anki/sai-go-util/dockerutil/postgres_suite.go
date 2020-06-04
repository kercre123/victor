package dockerutil

type PostgresSuite struct {
	Suite

	Password string
}

const (
	PostgresDefaultPassword = "password"
)

func (s *PostgresSuite) SetupSuite() {
	if s.Password == "" {
		s.Password = PostgresDefaultPassword
	}
	s.Start = func() (*Container, error) {
		return StartPostgresContainer(s.Password)
	}

	s.Suite.SetupSuite()
}
