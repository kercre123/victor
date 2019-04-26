package jdocssvc

// doc_specs.go - Defines the default document specs for deployment.
//
// XXX: Per-document CRUD permissions are not yet enforced, so the "AllowXyz"
// arrays are blank for now.

const defaultDocSpecs = `
{
  "Specs": [
    {
      "DocName":        "vic.Entitlements",
      "Description":    "Track promotions, eg from KickStarter campaign",
      "TableName":      "Acct-vic",
      "HasGDPRData":    false,
      "ExpectedMaxLen": 1024,
      "AllowCreate":    [],
      "AllowRead":      [],
      "AllowUpdate":    [],
      "AllowDelete":    []
    },
    {
      "DocName":        "vic.RobotOwners",
      "Description":    "History of accounts that have owned this robot",
      "TableName":      "Thng-vic",
      "HasGDPRData":    false,
      "ExpectedMaxLen": 1024,
      "AllowCreate":    [],
      "AllowRead":      [],
      "AllowUpdate":    [],
      "AllowDelete":    []
    },
    {
      "DocName":        "vic.AppTokens",
      "Description":    "Tokens that authorize external apps (eg SDK) to use Victor",
      "TableName":      "AcctThng-vic",
      "HasGDPRData":    false,
      "ExpectedMaxLen": 2048,
      "AllowCreate":    [],
      "AllowRead":      [],
      "AllowUpdate":    [],
      "AllowDelete":    []
    }
  ]
}
`
