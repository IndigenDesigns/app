#
# module.mk
#
# Copyright (C) 2013-2018 Studio Link
#

MOD		:= apponair
$(MOD)_SRCS	+= apponair.c account.c utils.c
$(MOD)_LFLAGS	+= -lm

include mk/mod.mk
