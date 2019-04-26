package dockerutil

import (
	"github.com/aws/aws-sdk-go/aws"
	"github.com/aws/aws-sdk-go/aws/credentials"
	"github.com/aws/aws-sdk-go/aws/session"
	"github.com/aws/aws-sdk-go/service/kinesis"
)

type KinesisSuite struct {
	Suite

	Kinesis  *kinesis.Kinesis
	Config   *aws.Config
	Endpoint string
}

func (s *KinesisSuite) SetupSuite() {
	s.Start = StartKinesisContainer
	s.Suite.SetupSuite()

	s.Endpoint = "http://" + s.Container.Addr()
	s.Config = &aws.Config{
		Region:      aws.String("us-east-1"),
		Endpoint:    aws.String(s.Endpoint),
		DisableSSL:  aws.Bool(true),
		Credentials: credentials.NewStaticCredentials("x", "x", ""),
	}
	s.Kinesis = kinesis.New(session.New(), s.Config)
}

func (s *KinesisSuite) CreateStream(name string) string {
	_, err := s.Kinesis.CreateStream(&kinesis.CreateStreamInput{
		StreamName: aws.String(name),
		ShardCount: aws.Int64(1),
	})
	if err != nil {
		s.T().Errorf("Unable to create stream %s: %s", name, err)
	}

	err = s.Kinesis.WaitUntilStreamExists(&kinesis.DescribeStreamInput{
		StreamName: aws.String(name),
	})
	if err != nil {
		s.T().Fatalf("Error waiting for stream %s: %s", name, err)
	}

	stream, err := s.Kinesis.DescribeStream(&kinesis.DescribeStreamInput{
		StreamName: aws.String(name),
	})
	if err != nil {
		s.T().Fatalf("Error describing stream %s: %s", name, err)
	}

	if len(stream.StreamDescription.Shards) == 0 {
		s.T().Fatalf("No shards in stream %s", name)
	} else {
		return *stream.StreamDescription.Shards[0].ShardId
	}
	return ""
}

func (s *KinesisSuite) DeleteStream(name string) {
	if _, err := s.Kinesis.DeleteStream(&kinesis.DeleteStreamInput{
		StreamName: aws.String(name),
	}); err != nil {
		s.T().Errorf("Error deleting stream %s: %s", name, err)
	}
}

func (s *KinesisSuite) PutRecords(stream string, recs [][]byte) {
	records := make([]*kinesis.PutRecordsRequestEntry, 0)
	for _, rec := range recs {
		records = append(records, &kinesis.PutRecordsRequestEntry{
			Data:         rec,
			PartitionKey: aws.String("TestPartition"),
		})
	}

	_, err := s.Kinesis.PutRecords(&kinesis.PutRecordsInput{
		StreamName: aws.String(stream),
		Records:    records,
	})
	if err != nil {
		s.T().Fatalf("Failed to put %d kinesis records in stream %s: %s", len(recs), stream, err)
	}
}

func (s *KinesisSuite) GetRecords(stream, shardID string) [][]byte {
	it := s.GetIterator(stream, shardID)
	recs := make([][]byte, 0)

	resp, err := s.Kinesis.GetRecords(&kinesis.GetRecordsInput{
		ShardIterator: aws.String(it),
	})

	if err != nil {
		s.T().Errorf("Could not get kinesis records for stream %s, shard %s: %s", stream, shardID, err)
	}
	for _, r := range resp.Records {
		recs = append(recs, r.Data)
	}
	return recs
}

func (s *KinesisSuite) GetIterator(stream, shardID string) string {
	it, err := s.Kinesis.GetShardIterator(&kinesis.GetShardIteratorInput{
		ShardId:           aws.String(shardID),
		ShardIteratorType: aws.String("TRIM_HORIZON"),
		StreamName:        aws.String(stream),
	})
	if err != nil {
		s.T().Errorf("Could not get shard iterator for stream %s, shard %s: %s", stream, shardID, err)
	}
	return *it.ShardIterator
}
