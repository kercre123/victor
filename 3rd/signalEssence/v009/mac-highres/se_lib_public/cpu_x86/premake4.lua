se.trace_enter()

--
-- cpu = x86 (_64?  _32?  hmm...)
function se_get_params_se_lib_public_cpu(premake_opts)
   local dict = se_empty_build_dict()

   --
   -- include dirs
   dict.include_dirs = {mmfx_root .. "/se_lib_public/cpu_x86"}

   return dict
end

se.trace_exit()

