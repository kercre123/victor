package jdocssvc

import (
	"github.com/anki/sai-go-util/metricsutil"
)

var (
	// The metrics registry for all service-specific metrics should
	// always be named "service.service-name", where service-name is
	// the same named used in logging, i.e. sai_chipper,
	// sai_token_service, etc....
	//
	// The service framework initializes that to be name of the binary
	// in snake case. While it would be ideal for this to be
	// initialized by the framework as well, it turns out to be too
	// difficult to do while still making metrics convenient to work
	// with.
	JdocsMetrics = metricsutil.NewRegistry("service.sai_jdocs")
)
