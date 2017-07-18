module HockeyApp
  def self.curl(url, headers={}, form_data={})
    header_args = headers.map do |k, v|
      %{-H "#{k}: #{v}"}
    end
    form_args = form_data.map do |k, v|
      "-F #{k}='#{v}'"
    end

    command = "/usr/bin/curl #{header_args.join(' ')} #{form_args.join(' ')} #{url}"
    puts command
    success = system command
    success || abort
  end

  def self.tokens
    tokens = {}

    # read from ~/.hockeyapprc
    rc_file = File.expand_path('~/.hockeyapprc')
    if ( File.exists? rc_file )
      File.open(rc_file, 'r').each_line do |line|
        # search for 'HOCKEYAPP_* variables
        m = /HOCKEYAPP_([A-Z]+)_TOKEN=[\"\']?(\w+)/.match(line.strip)
        next unless m

        key = m[1].downcase.to_sym

        tokens[key] = m[2]
      end if File.exists? rc_file
    end

    # check the environment
    tokens[:api] = ENV['HOCKEYAPP_API_TOKEN'] if ENV['HOCKEYAPP_API_TOKEN']
    tokens[:app] = ENV['HOCKEYAPP_APP_TOKEN'] if ENV['HOCKEYAPP_APP_TOKEN']

    tokens
  end

  def self.upload(ipa_file, dsym_file, options={}, token_params={})
    # curl \
    #  -F "status=2" \
    #  -F "notify=1" \
    #  -F "notes=Some new features and fixed bugs." \
    #  -F "notes_type=0" \
    #  -F "ipa=@hockeyapp.ipa" \
    #  -F "dsym=@hockeyapp.dSYM.zip" \
    #  -H "X-HockeyAppToken: 4567abcd8901ef234567abcd8901ef23" \
    #  https://rink.hockeyapp.net/api/2/apps/1234567890abcdef1234567890abcdef/app_versions/upload

    defaults = {  :notify => 1,
                  :notes_type => 1,
                  :status => 2 }
    params = defaults.dup
    params.merge!(options)

    params[:ipa] = "@#{ipa_file}"
    params[:dsym] = "@#{dsym_file}"

    api_tokens = self.tokens
    api_tokens.merge!(token_params)
    
    abort("Missing HockeyApp API token. An API token is required to upload builds") unless api_tokens[:api]

    url = %{https://rink.hockeyapp.net/api/2/apps/#{api_tokens[:app]}/app_versions/upload}
    headers = { 'X-HockeyAppToken' => api_tokens[:api] }
    curl(url, headers, params)
  end
  
end
