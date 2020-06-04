package certstore

import "context"

type RobotCertificateStore interface {
	Store(ctx context.Context, product, robotID string, cert []byte) error
}

type DummyRobotCertificateStore struct{}

func (s *DummyRobotCertificateStore) Store(ctx context.Context, product, robotID string, cert []byte) error {
	return nil
}
