Import("env") # type: ignore

#
# enable hardware floating point unit
#
flags = [
  "-mfpu=fpv4-sp-d16",
  "-mfloat-abi=hard"
]

env.Append(
  CCFLAGS = flags,
  LINKFLAGS = flags + ["-u", "_printf_float"]
)