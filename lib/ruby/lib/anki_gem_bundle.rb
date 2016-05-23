paths = [ 'twine/lib', 'plist/lib', 'rubyzip/lib' ]

paths.each do |gem_path|
  $LOAD_PATH.unshift File.join(File.dirname(__FILE__), gem_path)
end

