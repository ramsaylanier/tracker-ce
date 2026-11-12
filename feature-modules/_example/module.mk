# Example Feature Module — excluded from default builds (name starts with _).
# Build explicitly:
#   make all FEATURE_MODULE_DIRS="feature-modules/_example"
MODULE_NAME        := example-sdk
MODULE_TITLE       := SDK Example
MODULE_DESCRIPTION := Demonstrates Module SDK v2 hooks (not built by default)
MODULE_ORDER       := 90
MODULE_INSTANCE    := exampleSdkModule
MODULE_HEADER      := ExampleSdkModule.h
MODULE_SRCS        := ExampleSdkModule.cpp
