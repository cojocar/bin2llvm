require 'aruba/cucumber'

class TranslatorPaths
	def initialize(prefix_path='./')
		@prefix_path = prefix_path
	end

	def get(s)
		return File.join(@prefix_path, s)
	end

	def get_bin(s)
		return self.get(File.join('bin', s))
	end

	def get_lib(s)
		return self.get(File.join('lib', s))
	end

	def get_qemu()
		return self.get_bin(File.join('qemu-release', 'arm-s2e-softmmu', 'qemu-system-arm'))
	end

	def get_linker()
		return self.get_bin('linky')
	end

	def get_translator_so()
		return self.get_lib('translator.so')
	end

	def get_llvm_opt()
		return self.get_bin('opt')
	end

	def get_llvm_as()
		return self.get_bin('llvm-as')
	end

	def get_llvm_link()
		return self.get_bin('llvm-link')
	end

	def get_llvm_dis()
		return self.get_bin('llvm-dis')
	end

	def get_translator()
		return self.get_bin('bin2llvm.py')
	end
end

def bc_to_ll(filein, fileout)
	check_file_presence([filein], true)
	cmd=@path.get_llvm_dis + " " + filein
	if not fileout.nil?
		cmd=cmd + " -o " + fileout
	end
	run_simple(unescape(cmd), true, 20)
end

def ll_to_bc(filein, fileout)
	check_file_presence([filein], true)
	cmd=@path.get_llvm_as + " " + filein + " -o " + fileout
	run_simple(unescape(cmd), true, 20)
end

Before do
	if ENV.key?('BIN2LLVM_INSTALL_DIR')
		@path = TranslatorPaths.new(ENV['BIN2LLVM_INSTALL_DIR'])
	else
		@path = TranslatorPaths.new("../../bin2llvm-install")
	end
end


