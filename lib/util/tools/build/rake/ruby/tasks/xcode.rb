#
# Rakefile for RushHour build tasks
#

require 'time'

$LOAD_PATH.unshift File.join(File.dirname(__FILE__), '..', 'lib', 'plist', 'lib')
require 'plist'

XCTOOL = File.join(File.dirname(__FILE__), '..', '..', 'xctool', 'bin', 'xctool')
MP_PARSE = File.join(File.dirname(__FILE__), '..', '..', 'mpParse')
XCODEBUILD = '/usr/bin/xcodebuild'
PLIST_BUDDY = '/usr/libexec/PlistBuddy'
SECURITY = '/usr/bin/security'

class PlistBuddy
  attr_accessor :path
  def initialize(path)
    return nil unless File.exists? path
    @path = path
    self
  end

  def to_array(value)
    array_values = /^Array \{([^\{]*)\}$/.match(value.strip)
    return nil unless array_values

    array_values.captures[0].strip.split("\n").map { |v| v.strip }
  end

  def fetch(key_path,xml=false)
    return nil unless key_path
    opts = xml ? " -x " : ""
    value = `#{PLIST_BUDDY} #{opts} -c "Print :#{key_path}" "#{self.path}"`.strip
    return self.to_array(value) || value
  end

  def [](key_path)
    return nil unless key_path
    value = `#{PLIST_BUDDY} -c "Print :#{key_path}" "#{self.path}"`.strip
    return self.to_array(value) || value
  end

  def []=(key_path, value)
    return nil unless key_path
    system %{#{PLIST_BUDDY} -c "Set :#{key_path} #{value}" "#{self.path}"}
  end
end

class ProvisioningProfile
  attr_accessor :path
  def initialize(path)
    return nil unless File.exists? path
    @path = path
    return self
  end

  def fetch(key)
    `#{MP_PARSE} -f #{self.path} -o #{key}`.strip
  end

  def uuid
    @uuid ||= self.fetch(:uuid)
  end

  def app_id
    @app_id ||= self.fetch(:appid)
  end

  def bundle_id
    @bundle_id = self.app_id.split('.')[1..-1].join('.')
  end

  def team_id
    @team_id ||= self.fetch(:team_id)
  end

  def team_name
    @team_name ||= self.fetch(:team_name)
  end

  def environment
    @environment ||= self.fetch(:environment)
  end

  def type
    @type ||= self.fetch(:type)
  end

  def codesign_identity
    cert_type = 'iPhone Developer'
    if ( ['ad-hoc', 'appstore', 'universal'].include? self.type )
      cert_type = 'iPhone Distribution'
    end
    "#{cert_type}: #{self.team_name}"
  end
end


class XcodeProject
  attr_accessor :project_name, :project_root
  attr_accessor :xcodeproj, :path, :scheme, :workspace
  attr_accessor :product_name

  def initialize(project_root)
    raise AssertionError unless project_root
    self.project_root = project_root
    self
  end

  def buildtool
    #@build_command ||= XCTOOL
    @build_command ||= 'xcodebuild'
  end

  def xcodeproj
    result = Dir.glob(File.join(self.project_root, "*.xcodeproj"))
    #puts "find xcodeproj: #{result}"
    if (result.size > 0)
      @xcodeproj ||= result[0]
    else
      @xcodeproj ||= File.join(self.project_root, "#{File.basename(self.project_root, '.*')}.xcodeproj")
    end
    @xcodeproj
  end

  def scheme
    @scheme ||= self.project_name
  end

  def workspace
    return @workspace if @workspace
    @workspace = File.join(self.project_root, "#{self.scheme}.xcworkspace")
    @workspace = File.join(self.xcodeproj, 'project.xcworkspace') unless File.exists? @workspace
    @workspace
  end

  def project_name
    @project_name ||= File.basename(self.xcodeproj, '.*')
  end

  def product_name
    return @product_name if @product_name
    # include BuildServer.xcconfig if it exists
    build_xcconfig_path = File.join(self.project_root, 'Config', 'BuildServer.xcconfig') 
    xcconfig_option = File.exists?(build_xcconfig_path) ? "-xcconfig #{build_xcconfig_path}" : ""
    product_name_var = %x{#{self.buildtool} -workspace #{self.workspace} -scheme #{self.scheme} #{xcconfig_option} -showBuildSettings 2> /dev/null | grep " PRODUCT_NAME ="}
    m = / PRODUCT_NAME = (.+)/.match(product_name_var)
    @product_name ||= ( m ? m[1].strip : nil )
  end

  def info_plist_path
    @info_plist_path = File.join(project_root, "#{project_name}-Info.plist")
    p @info_plist_path
    @info_plist_path
  end

  def built_product_path(config, platform='iphoneos')
    File.join(self.built_products_dir(config, platform), self.product_name + '.app') 
  end

  def dsym_path(config, platform='iphoneos')
    self.built_product_path(config, platform) + '.dSYM'
  end

  def configuration_build_dir
    File.join(self.project_root, 'DerivedData')
  end

  def build_dir_settings(config, platform='iphoneos')
#    %{SYMROOT="#{self.configuration_build_dir}" DSTROOT="#{self.configuration_build_dir}" OBJROOT="#{self.configuration_build_dir}" }
    %{SYMROOT="#{self.configuration_build_dir}"  OBJROOT="#{self.configuration_build_dir}"  }
  end

  def build_dir
    #File.join(self.project_root, 'DerivedData', self.project_name, 'Build', 'Products')
    File.join(self.project_root, 'DerivedData')
  end

  def built_products_dir(config, platform='iphoneos')
    slug = config
    if ( platform && platform != 'i386' )
      slug += "-#{platform}"
    end
    File.join(self.build_dir, slug)
  end

  def build_args(config, profile_path=nil, platform='iphoneos')
    # derive codesign_identity and bundle_id from provisioning profile
    signing_args = ""
    if ( profile_path && File.exists?(profile_path) )
      profile = ProvisioningProfile.new(profile_path)
      signing_args = %{PROVISIONING_PROFILE="#{profile.uuid}" CODE_SIGN_IDENTITY="#{profile.codesign_identity}"}
      signing_args += %{ OTHER_CODE_SIGN_FLAGS="#{ENV['OTHER_CODE_SIGN_FLAGS']}" } if ENV['OTHER_CODE_SIGN_FLAGS'] 
    end

    # include BuildServer.xcconfig if it exists
    build_xcconfig_path = File.join(self.project_root, 'Config', 'BuildServer.xcconfig') 
    xcconfig_option = File.exists?(build_xcconfig_path) ? "-xcconfig #{build_xcconfig_path}" : ""

    %{-workspace #{self.workspace} -scheme #{self.scheme} -sdk #{platform} -configuration #{config} #{xcconfig_option} #{signing_args} #{self.build_dir_settings(config, platform)}}
  end

  def build(config, profile_path=nil, platform='iphoneos', command='build')
    # set Bundle ID based on Profile
    if ( profile_path && File.exists?(profile_path) )
      profile = ProvisioningProfile.new(profile_path)
      plist = PlistBuddy.new(self.info_plist_path)
      save_bundle_id = plist['CFBundleIdentifier']
      restore_bundle_id = false
      if ( save_bundle_id != profile.bundle_id )
        puts "Setting CFBundleIdentifier to #{profile.bundle_id} (was #{save_bundle_id})"
        plist['CFBundleIdentifier'] = profile.bundle_id
        restore_bundle_id = true
      end
    end

    # build!
    cmd = %{#{self.buildtool} #{self.build_args(config, profile_path, platform)} #{command} }

    success = false
    begin 
      puts cmd
      success = system cmd
    rescue
    ensure
      # restore profile
      if ( profile_path && restore_bundle_id )
        puts "Restoring CFBundleIdentifier to #{save_bundle_id}"
        plist['CFBundleIdentifier'] = save_bundle_id
      end
    end
    
    success || abort
  end

  def package(config, profile_path, dist_path, platform='iphoneos')
    profile = ProvisioningProfile.new(profile_path)
    abort unless profile

    app_path = self.built_product_path(config, platform)
    ipa_path = File.join(dist_path, "#{self.product_name}.ipa")
    dsym_path = self.dsym_path(config, platform)

    codesign_args = ENV['OTHER_CODE_SIGN_FLAGS'] || ""

    FileUtils.mkdir_p(dist_path)
    codesign_command = %{/usr/bin/codesign --force --preserve-metadata=identifier,entitlements,resource-rules --sign "#{profile.codesign_identity}" #{codesign_args} "#{app_path}"}

    puts codesign_command
    abort("codesign failed") unless system codesign_command

    command = %{xcrun -sdk #{platform} PackageApplication -v "#{app_path}" -o "#{ipa_path}" --embed "#{profile_path}" 1> /dev/null}

    success = false
    begin
      puts command
      success = system command
    rescue
    end 

    abort("Failed to create .ipa file") unless File.exists? ipa_path 

    dsym_dist_path = File.join(dist_path, File.basename(dsym_path))
    dsym_dist_zip = "#{dsym_dist_path}.zip"

    FileUtils.rm_f(dsym_dist_zip) if File.exists? dsym_dist_zip
    FileUtils.rm_rf(dsym_dist_path) if File.exists? dsym_dist_path
    FileUtils.cp_r(dsym_path, dsym_dist_path, :preserve => true) 
    success = system %{zip -r -X "#{dsym_dist_path}.zip" "#{dsym_dist_path}"} 

    success || abort
  end

  def product_versioned_name(config, profile_path)
    archive_name = %Q{#{self.product_name}-#{config}-#{File.basename(profile_path, '.*')}}
    version_h = File.join(self.project_root, '..', '..', 'basestation', 'version.h')
    return archive_name unless File.exists?(version_h)

    version = nil
    version_line = `grep APPVERSION "#{version_h}"`
    if ( version_line )
      version_info = version_line.split(' ').map { |e| e.strip }
      version = version_info.last.gsub!(/"/,'')
    end

    if ( version )
      archive_name = "#{archive_name}-#{version}"
    end

    return archive_name
  end

  def archive(config, profile_path, dist_path, platform='iphoneos')
    profile = ProvisioningProfile.new(profile_path)
    abort unless profile

    product_versioned_name = self.product_versioned_name(config, profile_path)
    app_path = self.built_product_path(config, platform)
    ipa_path = File.join(dist_path, "#{product_versioned_name}.ipa")
    dsym_path = self.dsym_path(config, platform)
    entitlements_path = File.join(dist_path, "#{product_versioned_name}.xcent")

    codesign_args = ENV['OTHER_CODE_SIGN_FLAGS'] || ""

    FileUtils.mkdir_p(dist_path)

    # replace embedded.profile with the desired profile
    FileUtils.cp profile_path, File.join(app_path, 'embedded.mobileprovision')

    # extract entitlements from profile
    embedded_profile_plist = entitlements_path + '.plist'
    %x{#{SECURITY} cms -D -i #{profile_path} > #{embedded_profile_plist}}
    entitlements = PlistBuddy.new(embedded_profile_plist).fetch("Entitlements", true) 
    File.open(entitlements_path, 'w') { |f| f.write(entitlements) }

    puts "Entitlements from embedded.mobileprovision"
    puts entitlements

    # re-sign app bundle
    developer_root = `xcode-select --print-path`.strip
    codesign_allocate_exe = File.join(developer_root, 'Platforms', 'iPhoneOS.platform', 'Developer', 'usr','bin', 'codesign_allocate')
    codesign_command = %{CODESIGN_ALLOCATE=#{codesign_allocate_exe} /usr/bin/codesign --preserve-metadata=resource-rules --force --verify --verbose --sign "#{profile.codesign_identity}" --entitlements "#{entitlements_path}" #{codesign_args} "#{app_path}"}

    puts codesign_command
    abort("codesign failed") unless system codesign_command

    # remove tmp entitlements info
    FileUtils.rm_f embedded_profile_plist
    FileUtils.rm_f entitlements_path

    # Create the .ipa package
    command = %{xcrun -sdk #{platform} PackageApplication -v "#{app_path}" -o "#{ipa_path}" --embed "#{profile_path}" 1> /dev/null}

    success = false
    begin
      puts command
      success = system command
    rescue
    end 

    abort("Failed to create .ipa file") unless File.exists? ipa_path 

    dsym_dist_path = File.join(dist_path, File.basename(dsym_path))
    dsym_dist_zip = File.join(dist_path, "#{product_versioned_name}.app.dSYM.zip")

    FileUtils.rm_f(dsym_dist_zip) if File.exists? dsym_dist_zip
    FileUtils.rm_rf(dsym_dist_path) if File.exists? dsym_dist_path
    FileUtils.cp_r(dsym_path, dsym_dist_path, :preserve => true) 
    success = system %{zip --quiet -r -X "#{dsym_dist_zip}" "#{dsym_dist_path}"} 

    abort("Failed to create dSYM archive") unless success && File.exists?(dsym_dist_zip)

    # create xcarchive
    # Use the config + version.h info (if available) to derive the version name
    archive_name = "#{product_versioned_name}.xcarchive"
    archive_path = File.join(dist_path, archive_name)

    archive_applications_path = File.join(archive_path, 'Products', 'Applications')
    FileUtils.mkdir_p(archive_applications_path)

    archive_dsyms_path = File.join(archive_path, 'dSYMs')
    FileUtils.mkdir_p(archive_dsyms_path)
    
    # copy App bundle into xcarchive
    FileUtils.cp_r(app_path, archive_applications_path, :preserve => true, :verbose => true)

    # copy dSYM into xcarchive 
    FileUtils.cp_r(dsym_path, archive_dsyms_path, :preserve => true, :verbose => true)

    # Create xcarchive Info.plist
    info_plist = PlistBuddy.new(self.info_plist_path)
    archive_application_path = File.join('Applications', File.basename(app_path))
    icon_files = info_plist['CFBundleIcons:CFBundlePrimaryIcon:CFBundleIconFiles']
    icon_paths = icon_files.map { |f| File.join(archive_application_path, f) }
    app_properties = {
      'ApplicationPath' => archive_application_path,
      'CFBundleIdentifier' => info_plist['CFBundleIdentifier'],
      'CFBundleShortVersionString' => info_plist['CFBundleShortVersionString'],
      'CFBundleVersion' => info_plist['CFBundleVersion'],
      'IconPaths' => icon_paths,
      'SigningIdentity' => profile.codesign_identity,
    }

    archive_info = {
      'ApplicationProperties' => app_properties,
      'ArchiveVersion' => 2,
      'CreationDate' => Time.now.utc.iso8601,
      'Name' => File.basename(archive_application_path, '.*'),
      'SchemeName' => self.scheme,
    }

    archive_plist_path = File.join(archive_path, 'Info.plist')
    plist = archive_info.to_plist
    File.open(archive_plist_path, 'w') { |f| f.write(plist) }

    abort("Failed to create xcarchive Info.plist") unless File.exists? archive_plist_path

    # create zipped version of xcarchive for build server download
    archive_zip = "#{archive_path}.zip"
    success = system %{zip --quiet -r -X "#{archive_zip}" "#{archive_path}"} 

    abort("Failed to create xcarchive zip") unless success && File.exists?(archive_zip)

    return true
  end

  def test(opts={})
    options = opts.dup
    output_dir = options.delete(:output_dir)
    output_command = ""
    if ( output_dir )
      output_path = File.join(output_dir, "#{self.scheme}-tests.xml")
      output_command = "> #{output_path}" 
    end

    # process options for unit tests
    test_option_keys = [:only, :test_sdk, :parallelize, :logic_test_bucket_size]
    test_options = {}
    test_option_keys.each do |key|
      v = options.delete(key)
      test_options[key] = v if v
    end

    options_string = options.map { |k, v| "-#{k} #{v}" }.join(" ")
    test_options_string = test_options.map { |k, v| "-#{k} #{v}" }.join(" ")
  
    command = %Q{#{XCTOOL} -workspace #{self.workspace} -scheme #{self.scheme} -sdk iphonesimulator #{options_string} test #{test_options_string} #{output_command} #{self.build_dir_settings('Release')}}
    system command
  end
end

