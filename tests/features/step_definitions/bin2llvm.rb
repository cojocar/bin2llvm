require 'tempfile'

Given(/^the binary "(.*?)" of type "(.*?)"$/) do |binary, binary_type|
	@input_binary_path="./tests/"+binary
	@input_binary_path=File.absolute_path(@input_binary_path)
	#announce_or_puts("input_binary_path: " + @input_binary_path)
	check_file_presence([@input_binary_path], true)
	@binary_type=binary_type
	@binary_entry=nil
	@binary_load=nil
end

Given(/^the entry point "(.*?)"$/) do |addr|
	@binary_entry=addr
end

Given(/^the load address "(.*?)"$/) do |addr|
	@binary_load=addr
end

When(/^translator runs with random output directory$/) do
	@tmp_dir=Dir.mktmpdir('translator-testing-'+
						  File.basename(@input_binary_path)+'-')
	announce_or_puts('Using ' + @tmp_dir + ' as temporary directory')
	cmd=@path.get_translator+
		" --type " + @binary_type +
		" --file " + @input_binary_path +
		" --temp-dir " + @tmp_dir
	if @binary_entry.nil?
		@binary_entry = "0x0"
	end
	if not @binary_load.nil?
		cmd = cmd + " --load-address " + @binary_load
	end
	cmd = cmd + " --entry " + @binary_entry
	#announce_or_puts("Running: " + cmd)
	run_simple(unescape(cmd), true, 100)
	#announce_or_puts('done translation')
	#cd(@tmp_dir)
end

Then(/^the out file "(.*?)" should not be empty$/) do |file|
	if File.size(@test_dir+'/'+file) == 0
		raise "empty file"
	end
end

Then(/^an out file named "(.*?)" should exist$/) do |file|
	file_path=File.absolute_path(@tmp_dir+"/"+file)
	check_file_presence([file_path], true)
end

Then /^the out file "([^"]*)" should (not )?contain "([^"]*)"$/ do |file, expect_match, partial_content|
	file_path=File.absolute_path(@tmp_dir+"/"+file)
	check_file_content(file_path, Regexp.compile(Regexp.escape(partial_content)), !expect_match)
end

Then /^the out file "(.*?)" should (not )?contain:$/ do |file, expect_match, partial_content|
	file_path=File.absolute_path(@tmp_dir+"/"+file)
	check_file_content(file_path, Regexp.compile(Regexp.escape(partial_content)), !expect_match)
end

Given(/^llvm file of "(.*?)"$/) do |file|
	bc_file_path=File.absolute_path(@tmp_dir+"/"+file)
	bc_to_ll(bc_file_path, nil)
end

Given(/^unified "(.*?)" in "(.*?)"$/) do |file_glob, file_out|
	cmd="cat " + @tmp_dir + "/" + file_glob + " > " + @tmp_dir + "/" + file_out
	cmd="bash -c \"" + cmd + "\""
	run_simple(cmd, true, 20)
end
