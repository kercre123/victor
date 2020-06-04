package jdocs

// clients.go
//
// Types and helpers to track the type of clients that connect to JDOCS service.
// Example client types include Chewie App, Victor Robot, Token Service, Chipper
// Weather Service.

//////////////////////////////////////////////////////////////////////

// Use string, rather than enum, for easier JSON wrangling
type ClientType string

const (
	ClientTypeUnknown               = "ClientTypeUnknown"
	ClientTypeSuperuser             = "ClientTypeSuperuser"
	ClientTypeTokenService          = "ClientTypeTokenService"
	ClientTypeChewieApp             = "ClientTypeChewieApp"
	ClientTypeSDKApp                = "ClientTypeSDKApp"
	ClientTypeChipperTimerService   = "ClientTypeChipperTimerService"
	ClientTypeChipperWeatherService = "ClientTypeChipperWeatherService"
	ClientTypeVicRobot              = "ClientTypeVicRobot"
)
