GCC = g++
ARCHIVER = ar
WARNS = -Wall -Werror
GCC_FLAGS = -fPIC
BUILD_FLAGS = -c
SHARED_FLAGS = -shared
SHARED_SONAME = -Wl,-soname
OBJECTS = htb64.o ht_file_versioning.o
OTM_FLAGS = -O3

ifdef DEBUG
  OTM_FLAGS = 
  GCC = g++ -ggdb -gstabs+
endif

LINUX_B_DIR = ltarget
LINUX_S_DIR = starget
LINUX_O_DIR = otarget
LINUX_T_DIR = test
LINUX_PACK_DIR = pck
LINUX_LT_DIR = /usr/lib
LINUX_IT_DIR = /usr/include
LINUX_PY_DIR = build

TOBJECTS = $(addprefix $(LINUX_O_DIR)/, $(OBJECTS))

FORCE_ERASE = rm -rf
LINKER = ln -s
COPYER = cp -a
FPM = fpm
MODE = chmod 555

MAJOR_V = 1
MINOR_V = 0

NAME = ht_file_versioning
STATIC_NAME = lib$(NAME).a
SHARED_S_NAME = lib$(NAME).so
SHARED_T_NAME = $(SHARED_S_NAME).$(MAJOR_V)
SHARED_C_NAME = $(SHARED_T_NAME).$(MAJOR_V)

FPM_NAME_PY = -n py_$(NAME)
FPM_NAME_LIB = -n lib_$(NAME)
FPM_VER = -v $(MAJOR_V).$(MINOR_V)
FPM_DEB = -t deb
FPM_RPM = -t rpm
FPM_PYTHON = -s python
FPM_DIR = -s dir
FPM_PY_IN_FILE = setup.py
FPM_DIR_ALL = -C $(LINUX_PACK_DIR) .

TARGETS_HEADERS = $(LINUX_IT_DIR)/ht_file_versioning.h $(LINUX_IT_DIR)/htb64.h $(LINUX_IT_DIR)/one_at_time.hpp
COPY_HEADERS = ht_file_versioning.h htb64.h one_at_time.hpp

GOOGLE_TEST_DIR = fused-src
GOOGLE_TEST_LIBS = -lpthread
TESTS = test.cpp
TEST_NAME = test
TEST_BUILD_ADITIONALS = -ggdb -gstabs+

#ALL
all: python_modules linux_libraries test


#DIRS
linux_b_dir:
	if test ! -d $(LINUX_B_DIR) ; then $(FORCE_ERASE) $(LINUX_B_DIR) && mkdir $(LINUX_B_DIR) ; fi

linux_s_dir:
	if test ! -d $(LINUX_S_DIR) ; then $(FORCE_ERASE) $(LINUX_S_DIR) && mkdir $(LINUX_S_DIR) ; fi

linux_o_dir:
	if test ! -d $(LINUX_O_DIR) ; then $(FORCE_ERASE) $(LINUX_O_DIR) && mkdir $(LINUX_O_DIR) ; fi

linux_t_dir:
	if test ! -d $(LINUX_T_DIR) ; then $(FORCE_ERASE) $(LINUX_T_DIR) && mkdir $(LINUX_T_DIR) ; fi

#PYTHON MODULES
python_modules:
	python setup.py build

test: linux_lib_static linux_t_dir
	@echo "Running TESTS from google test" $(GOOGLE_TEST_DIR) 
	$(GCC) $(TESTS) $(LINUX_S_DIR)/$(STATIC_NAME) -o $(LINUX_T_DIR)/$(TEST_NAME) -I $(GOOGLE_TEST_DIR) $(GOOGLE_TEST_LIBS) $(TEST_BUILD_ADITIONALS)
	cd $(LINUX_T_DIR) && ./$(TEST_NAME)

#LINUX LIBS
linux_libraries: linux_lib_static linux_lib_dynamic

linux_lib_static: $(TOBJECTS)
	$(ARCHIVER) rcs $(LINUX_S_DIR)/$(STATIC_NAME) $^

linux_lib_dynamic: $(TOBJECTS)
	$(GCC) $(SHARED_FLAGS) $(SHARED_SONAME),$(SHARED_T_NAME) -o $(LINUX_B_DIR)/$(SHARED_C_NAME) $^ -lc

	if test -e $(LINUX_B_DIR)/$(SHARED_T_NAME) ; then $(FORCE_ERASE) $(LINUX_B_DIR)/$(SHARED_T_NAME) ; fi
	$(LINKER) $(SHARED_C_NAME) $(LINUX_B_DIR)/$(SHARED_T_NAME)

	if test -e $(LINUX_B_DIR)/$(SHARED_S_NAME) ; then $(FORCE_ERASE) $(LINUX_B_DIR)/$(SHARED_S_NAME) ; fi
	$(LINKER) $(SHARED_C_NAME) $(LINUX_B_DIR)/$(SHARED_S_NAME)

$(TOBJECTS): linux_b_dir linux_s_dir linux_o_dir

$(LINUX_O_DIR)/%.o: %.cpp
	$(GCC) $(WARNS) $(OTM_FLAGS) $(GCC_FLAGS) $(BUILD_FLAGS) $< -o $@


#INSTALL
install: install_linux_static install_linux_dynamic install_python_modules install_linux_headers

install_python_modules:
	python setup.py install

install_linux_static:
	$(MODE) $(LINUX_S_DIR)/*
	$(COPYER) $(LINUX_S_DIR)/* $(LINUX_LT_DIR)

install_linux_dynamic:
	$(MODE) $(LINUX_B_DIR)/*
	$(COPYER) $(LINUX_B_DIR)/* $(LINUX_LT_DIR)

install_linux_headers:
	$(COPYER) $(COPY_HEADERS) $(LINUX_IT_DIR)
	$(MODE) $(TARGETS_HEADERS)


#PACKAGES
packs: pack_python_modules pack_linux_lib

pack_python_modules:
	$(FPM) $(FPM_PYTHON) $(FPM_DEB) $(FPM_NAME_PY) $(FPM_VER) $(FPM_PY_IN_FILE)
	$(FPM) $(FPM_PYTHON) $(FPM_RPM) $(FPM_NAME_PY) $(FPM_VER) $(FPM_PY_IN_FILE)

pack_linux_lib: sys_organize
	$(FPM) $(FPM_DIR) $(FPM_DEB) $(FPM_NAME_LIB) $(FPM_VER) $(FPM_DIR_ALL)
	$(FPM) $(FPM_DIR) $(FPM_RPM) $(FPM_NAME_LIB) $(FPM_VER) $(FPM_DIR_ALL)

sys_organize: linux_libraries
	if test ! -d $(LINUX_PACK_DIR) ; then $(FORCE_ERASE) $(LINUX_PACK_DIR) ; mkdir $(LINUX_PACK_DIR) ; fi
	if test ! -d $(LINUX_PACK_DIR)/usr ; then $(FORCE_ERASE) $(LINUX_PACK_DIR)/usr ; mkdir $(LINUX_PACK_DIR)/usr ; fi
	if test ! -d $(LINUX_PACK_DIR)/usr/lib ; then $(FORCE_ERASE) $(LINUX_PACK_DIR)/usr/lib ; mkdir $(LINUX_PACK_DIR)/usr/lib ; fi
	$(COPYER) $(LINUX_B_DIR)/* $(LINUX_S_DIR)/* $(LINUX_PACK_DIR)/usr/lib

#HELPERS
help:
	@echo "make:"
	@echo "  all:"
	@echo "    linux_libraries:"
	@echo "      linux_lib_static"
	@echo "      linux_lib_dynamic"
	@echo "    python_modules"
	@echo "    test:"
	@echo "      linux_libraries"
	@echo "  test:"
	@echo "    linux_libraries"
	@echo "  install:"
	@echo "    install_python_modules"
	@echo "    install_linux_static"
	@echo "    install_linux_dynamic"
	@echo "    install_linux_headers"
	@echo "  packs:"
	@echo "    pack_python_modules"
	@echo "    pack_linux_lib:"
	@echo "      linux_libraries"
	@echo "  clean"

clean:
	$(FORCE_ERASE) $$(find -name "*.deb")
	$(FORCE_ERASE) $$(find -name "*.rpm")
	$(FORCE_ERASE) $(LINUX_T_DIR)
	$(FORCE_ERASE) $(LINUX_PY_DIR)
	$(FORCE_ERASE) $(LINUX_B_DIR)
	$(FORCE_ERASE) $(LINUX_S_DIR)
	$(FORCE_ERASE) $(LINUX_O_DIR)
	$(FORCE_ERASE) $(LINUX_PACK_DIR)
