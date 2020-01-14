Import("env")
import subprocess

# access to global construction environment
print (env)

# Git revision
version = subprocess.check_output(["git", "rev-parse", "HEAD"]).strip()

env.Replace(CPPDEFINES=[
  ("GITVERSION", version)
])

# Dump construction environments (for debug purpose)
#print env.Dump()