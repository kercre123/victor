package hstore

type Record struct {

	// Hypothesis is the ASR result
	Hypothesis string `json:"hypothesis"`

	// Service is the third-party service use for ASR and intent-matching
	// valid services are "dialogflow", "bing", "lex"
	Service string `json:"intent_service"`

	// Confidence is the ASR vendor's confidence in the hypothesis result
	Confidence float32 `json:"confidence"`

	// IntentMatch is the intent-result from
	IntentMatch string `json:"intent_matched"`

	// IntentConfidence is the confidence of the intent-match result
	IntentConfidence float32 `json:"intent_confidence"`

	// RawEntity is the entity result of the intent, stored as a json string
	RawEntity string `json:"raw_entity"`

	// ParsedEntity is the result chipper-fn returned after processing the RawEntity
	ParsedEntity string `json:"parsed_entity"`

	// LangCode of the request
	LangCode string `json:"lang_code"`

	// Mode of the request
	// valid modes: KG, GAMES, VOICE_COMMAND
	Mode string `json:"mode"`

	// RequestTime is the timestamp of request in YYYY-MM-DD-HH
	RequestTime string `json:"request_time"`

	// RawKgResponse is the raw response from Houndify
	RawKgResponse string `json:"raw_kg_response"`

	// KgResponse is the knowledge-graph response returned to chipper's client
	KgResponse string `json:"kg_response"`
}
