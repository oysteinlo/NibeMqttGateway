Import("env")
import subprocess

# access to global construction environment
print env

# Git revision
revision = subprocess.check_output(["git", "rev-parse", "HEAD"]).strip()

my_flags = env.ParseFlags(env['BUILD_FLAGS'])
defines = {k: v for (k, v) in my_flags.get("CPPDEFINES")}
# print defines

env.Replace(CPPDEFINES=[
  ("VERSION", revision)
])

# Dump construction environments (for debug purpose)
print env.Dump()