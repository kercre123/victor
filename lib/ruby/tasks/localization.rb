require File.join(File.dirname(__FILE__), '..', 'lib', 'anki_gem_bundle')

require 'fileutils'
require 'twine'

module Twine
  TWINE_CMD = File.join(File.dirname(__FILE__), '..', 'lib', 'twine', 'bin', 'twine')

  def self.make_opt(key, value)
    dash_key = key.to_s.sub(/_/,'-')
    value_is_bool = (!!value == value)
    arg_value = value_is_bool ? "" : value
    "--#{dash_key} #{arg_value}"
  end

  def self.compile_options(opts)
    cmd_opts = opts.map { |k,v| self.make_opt(k, v) }
    cmd_opts.join(' ')
  end

  def self.execute(cmd, *args)
    cmd = %{#{TWINE_CMD} #{cmd} #{args.join(' ')}}
    puts cmd
    success = system(cmd)
    abort unless success
    success
  end

  def self.consume_all_string_files(strings_db, localization_root, opts={})
    abort("Missing .txt file for strings") unless strings_db && File.exists?(strings_db)
    abort("Missing Localization root dir") unless localization_root && File.exists?(localization_root)

    cmd_opts = self.compile_options(opts)
    self.execute('consume-all-string-files', strings_db, localization_root, cmd_opts)
  end

  def self.consume_string_file(strings_db, strings_file, opts={})
    abort("Missing .txt file for strings") unless strings_db && File.exists?(strings_db)

    cmd_opts = self.compile_options(opts)
    self.execute('consume-string-file', strings_db, strings_file, cmd_opts)
  end

  def self.generate_all_string_files(strings_db, localization_root, opts={})
    abort("Missing .txt file for strings") unless strings_db && File.exists?(strings_db)
    abort("Missing Localization root dir") unless localization_root && File.exists?(localization_root)

    cmd_opts = self.compile_options(opts)
    self.execute('generate-all-string-files', strings_db, localization_root, cmd_opts)
  end

  def self.generate_string_file(strings_db, strings_file, opts={})
    abort("Missing .txt file for strings") unless strings_db && File.exists?(strings_db)

    cmd_opts = self.compile_options(opts)
    self.execute('generate-string-file', strings_db, strings_file, cmd_opts)
  end


end

class Localization
  attr_accessor :project_root
  attr_accessor :localization_dir, :xib_dir
  attr_accessor :strings_db
  attr_accessor :src_lang

  def initialize(project_root, localization_dir='LocalizedResources', src_lang='Base')
    @project_root = project_root
    @src_lang = src_lang
    @localization_dir = localization_dir
    @strings_db = File.join(project_root, 'LocalizedStringsDB.txt')
    self
  end

  def localization_root
    File.join(self.project_root, self.localization_dir)
  end

  def xib_root
    File.join(self.project_root, self.xib_dir)
  end

  def src_lang_dir
    self.src_lang + '.lproj'
  end

  def code
    resource_dir = File.join(self.project_root, self.localization_dir)
    localized_resource_dir = File.join(resource_dir, "#{self.src_lang}.lproj")
    FileUtils.mkdir_p localized_resource_dir unless File.exists? localized_resource_dir

    Dir.chdir(project_root) do |d|
      # Regenerate Localizable.strings from source files.
      %x{find -E . -iregex '.*\.(m|mm)$' -print0 | xargs -0 genstrings -s "AnkiLocalizedString" -o #{localized_resource_dir}}
      # Append additional strings to Localizable.strings.  Note that we need to
      # skip over each file's BOM while still preserving their UTF-16 encoding.
      # %x{tail -c +3 #{localized_resource_dir}/Example.strings >> #{localized_resource_dir}/Localizable.strings}
    end
  end

  def xib
    FileList[File.join(self.project_root, '**', "Base.lproj", '*.xib')].each do |f|
      strings_file = f + '.strings'
      %x{ibtool --export-strings-file #{strings_file} #{f}}
    end
    # orphaned .xib.strings files
    FileList[File.join(self.project_root, '**', "#{self.src_lang}.lproj", '*.xib.strings')].each do |f|
      FileUtils.rm_f unless File.exists? f.sub(/\.strings$/, '')
    end
  end

  def clean
    pattern = File.join(self.project_root, self.localization_dir, "#{self.src_lang}.lproj", '*.strings')
    exclude = []

    FileList[pattern].exclude(*exclude).each do |f|
      FileUtils.rm_f f
    end
  end

  def genstrings
    self.code
  end

  # twine support
  def strings_db
    @strings_db ||= File.join(self.localization_root, 'strings.txt')
  end

  def consume(strings_db=nil)
    strings_db ||= self.strings_db

    # create twine strings db
    FileUtils.touch strings_db

    # read files from Base localization
    self.consume_code(strings_db)
  end

  # use twine to create/update a .strings file from Base.lproj
  def consume_code(strings_db=nil)
    opts = {
      :developer_language => self.src_lang,
      :consume_all => true,
      :consume_comments => true
    }
    Twine.consume_all_string_files(strings_db, self.localization_root, opts)

    # consume Base.lproj/*.strings files and map them to 'en' in TwineDB 
    src_lang_path = File.join(self.localization_root, self.src_lang + '.lproj')
    FileList[File.join(src_lang_path, '*.strings')].each do |file|
      Twine.consume_string_file(strings_db, file, {:lang => 'en'}) 
    end
  end

  def consume_translations(strings_db=nil, translations_root)
    abort("Translations folder not found") unless File.exists? translations_root
    strings_db ||= self.strings_db

    opts = {
      :consume_all => true,
      :consume_comments => true
    }
    Twine.consume_all_string_files(strings_db, translations_root, opts)
  end

  def consume_xib(strings_db=nil)
    xib_strings_files = FileList[File.join(self.xib_root, '*.strings')]
    xib_strings_files.each do |f|
      Twine.consume_string_file(strings_db, f)
    end
  end

  def generate(strings_db=nil)
    strings_db ||= self.strings_db

    # create twine strings db
    FileUtils.touch strings_db

    # generate Localizable.strings files for each translation in twine DB
    self.generate_code(strings_db)
  end

  def generate_code(strings_db)
    Twine.generate_all_string_files(strings_db, self.localization_root)
  end

  def generate_xib(strings_db)
    localized_xib_files = FileList[File.join(self.xib_root, '*.xib')]
    localized_xib_files.each do |f|
      strings_file = File.join(File.dirname(f), File.basename(f, '.xib') + '.strings')
      Twine.consume_string_file(strings_db, string)
    end
  end

  GEN_TRANSLATION = File.join(File.dirname(__FILE__), '..', '..', 'GenerateNewStringLocalization.py')
  def create_fake_translation(lang, translations_root)
    # copy Base.lproj to new lang
    base_lang_path = File.join(self.localization_root, 'Base.lproj')
    lang_path = File.join(translations_root, "#{lang}.lproj")
    FileUtils.mkdir_p(lang_path)
    FileList[File.join(base_lang_path, '*.strings')].each do |strings_file|
      basename = File.basename(strings_file)
      target = File.join(lang_path, basename)
      system = %{#{GEN_TRANSLATION} #{strings_file} #{target}}
    end
  end
end
