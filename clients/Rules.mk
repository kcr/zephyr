sp := $(sp).x
dirstack_$(sp) := $(d)
d := $(dir)

dir := $(d)/zaway
include $(d)/zaway/Rules.mk

dir := $(d)/zctl
include $(d)/zctl/Rules.mk

dir := $(d)/zleave
include $(dir)/Rules.mk

dir := $(d)/zlocate
include $(dir)/Rules.mk

dir := $(d)/znol
include $(dir)/Rules.mk

dir := $(d)/zshutdown_notify
include $(dir)/Rules.mk

dir := $(d)/zstat
include $(dir)/Rules.mk

dir := $(d)/zwrite
include $(dir)/Rules.mk

d := $(dirstack_$(sp))
sp := $(basename $(sp))
