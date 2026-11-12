# Include from core/makefile after OBJ_DIR / INCLUDES / COMPILER_OPTIONS are set.
# Pass FEATURE_MODULE_DIRS as a space-separated list of absolute or relative paths.
#
# Each module directory must contain module.mk:
#   MODULE_NAME     := performance-fx-scenes
#   MODULE_INSTANCE := performanceFxScenes
#   MODULE_HEADER   := PerformanceFxScenes.h
#   MODULE_SRCS     := PerformanceFxScenes.cpp   # optional; defaults to *.cpp
#   MODULE_ORDER    := 10                        # optional; sorts registry + addPage order

FEATURE_MODULE_DIRS ?=

FEATURE_MODULE_OBJS :=
GENERATED_FEATURE_MODULE_REGISTRY := $(BUILD_DIR)/feature_modules_registry.cpp
GENERATED_FEATURE_MODULE_REGISTRY_OBJ := $(OBJ_DIR)/feature_modules_registry.o
# Stamp tracks the selected module-dir list so changing FEATURE_MODULE_DIRS
# regenerates the registry even when no module.mk file changed.
FEATURE_MODULE_DIRS_STAMP := $(BUILD_DIR)/feature_modules_dirs.stamp
FEATURE_MODULE_DIRS_NORM := $(strip $(foreach _d,$(FEATURE_MODULE_DIRS),$(abspath $(_d))))

# $(1) = MODULE_NAME, $(2) = absolute source path
define _feature_module_add_src
FEATURE_MODULE_OBJS += $(OBJ_DIR)/ext/$(1)/$(basename $(notdir $(2))).o
$(OBJ_DIR)/ext/$(1)/$(basename $(notdir $(2))).o: $(2)
	@mkdir -p $$(dir $$@)
	$$(TOOLCHAIN)c++ $$(DEFINES) $$(INCLUDES) $$(CPP_OPTIONS) $$(COMPILER_OPTIONS) -o $$@ $$<
endef

# $(1) = module directory. Paths from $(1) are expanded by $(call) immediately
# so they are not clobbered by the next foreach iteration.
define _feature_module_load
MODULE_NAME :=
MODULE_INSTANCE :=
MODULE_HEADER :=
MODULE_SRCS :=
MODULE_ORDER :=
MODULE_DEFINES :=
include $(abspath $(1))/module.mk
INCLUDES += -I$(abspath $(1))
# Bake MODULE_DEFINES into DEFINES immediately (recursive += would lose them when
# the next module clears MODULE_DEFINES).
ifneq ($$(strip $$(MODULE_DEFINES)),)
DEFINES := $$(DEFINES) $$(MODULE_DEFINES)
endif
_fm_name := $$(MODULE_NAME)
_fm_srcs := $$(MODULE_SRCS)
ifeq ($$(_fm_srcs),)
_fm_srcs := $$(notdir $$(wildcard $(abspath $(1))/*.cpp))
endif
$$(foreach _src,$$(_fm_srcs),$$(eval $$(call _feature_module_add_src,$$(_fm_name),$(abspath $(1))/$$(_src))))
endef

ifneq ($(strip $(FEATURE_MODULE_DIRS)),)
$(foreach _d,$(FEATURE_MODULE_DIRS),$(eval $(call _feature_module_load,$(_d))))
endif

.PHONY: _always_check_feature_module_dirs
_always_check_feature_module_dirs:

$(FEATURE_MODULE_DIRS_STAMP): _always_check_feature_module_dirs
	@mkdir -p $(dir $@)
	@if ! printf '%s\n' '$(FEATURE_MODULE_DIRS_NORM)' | cmp -s - $@ 2>/dev/null; then \
		printf '%s\n' '$(FEATURE_MODULE_DIRS_NORM)' > $@; \
		rm -f $(GENERATED_FEATURE_MODULE_REGISTRY) $(GENERATED_FEATURE_MODULE_REGISTRY_OBJ); \
	fi

$(GENERATED_FEATURE_MODULE_REGISTRY): modules/generate_registry.py \
		$(FEATURE_MODULE_DIRS_STAMP) \
		$(foreach _d,$(FEATURE_MODULE_DIRS),$(abspath $(_d))/module.mk)
	@mkdir -p $(dir $@)
	# Drop the .o so a same-second stamp/cpp rewrite cannot leave a stale object.
	@rm -f $(GENERATED_FEATURE_MODULE_REGISTRY_OBJ)
	python3 modules/generate_registry.py -o $@ $(FEATURE_MODULE_DIRS_NORM)

$(GENERATED_FEATURE_MODULE_REGISTRY_OBJ): $(GENERATED_FEATURE_MODULE_REGISTRY)
	@mkdir -p $(dir $@)
	$(TOOLCHAIN)c++ $(DEFINES) $(INCLUDES) $(CPP_OPTIONS) $(COMPILER_OPTIONS) -o "$@" "$<"
