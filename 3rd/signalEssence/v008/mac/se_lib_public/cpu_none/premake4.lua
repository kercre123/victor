
--
-- cpu = none
function se_get_params_se_lib_public_cpu(premake_opts)
   local dict = se_empty_build_dict()
   
   se.trace_enter()

   --
   -- include dirs
   dict.include_dirs = {mmfx_root .. "/se_lib_public/cpu_none"}

   se.trace_exit()
   return dict
end



