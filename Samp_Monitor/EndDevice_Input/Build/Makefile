##########################################################################
#  * (C) Tokyo Cosmos Electric, Inc. (TOCOS) - 2013 all rights reserved.
#
# TOCOS MAKEFILE TEMPLATE (for ToCoNet application build)
# ToCoNet のアプリケーションを作成するための Makefile です。
#
# ディレクトリに格納され使用します（サンプルアプリ参照）
#  {SDKROOT}/ToCoNet/{ApplicationRoot}/{FirmwareType}/Build/
##########################################################################

##########################################################################
### Chip selection (do not edit here)
# チップの選択を行います。指定が無い場合は、外部指定または chipsel.mkの規
# 定値となります。この変数はファイル名やフォルダ名の決定にも使用されます。
#
#TWE_CHIP_MODEL ?= JN5148
TWE_CHIP_MODEL ?= JN5164
#
# また TARGET_DIR 変数に .. 、PROJNAME に ../.. のディレクトリ名を格納
# します。
#
include ../../../../MkFiles/chipsel.mk
##########################################################################

### Version information
# ファイル名生成のためのバージョン定義を読み込みます。
# - VERSION_MAIN, VERSION_SUB, VERSION_VAR
# - ファームウェア中でのバージョン番号を利用する目的で ../Source/Version.h
#   を生成します。
#
include ../Version.mk

### Build Names
# 出力ファイル名のプレフィックスを決めます。指定しなければ、../..の
# ディレクトリ名(PROJNAMEの事前設定値)が使用されます。
# - (../..dir名)_(..dir名)_(JENNIC_CHIP)_(VERSION).
#
#PROJNAME=MyProject

### Source files
# コンパイル対象のソースコードを列挙します。
# - パスの指定は、APP_COMMON_SOURCE_DIR_ADD? で行います。
# - 以下のパスは事前定義されています。
#   ../Source
#   ../Common/Source
#
APPSRC += $(TARGET_DIR).c
APPSRC += common.c
APPSRC += flash.c

APPSRC += adc.c
APPSRC += SMBus.c
APPSRC += BH1715.c
APPSRC += SHT21.c
APPSRC += ADT7410.c
APPSRC += MPL115A2.c
APPSRC += LIS3DH.c
APPSRC += sensor_driver.c

APPSRC += Interactive.c
APPSRC += input_string.c

APPSRC += sercmd_ascii.c
APPSRC += sercmd_chat.c
APPSRC += sercmd_plus3.c
APPSRC += sercmd_binary.c

APPSRC += ProcessEv_Standard.c
APPSRC += ProcessEv_Botton.c
APPSRC += ProcessEv_Config.c
APPSRC += ProcessEv_Door_Timer.c
APPSRC += ProcessEv_Door_Timer_Nwk.c
APPSRC += ProcessEv_SHT21.c
APPSRC += ProcessEv_ADT7410.c
APPSRC += ProcessEv_MPL115A2.c
APPSRC += ProcessEv_LIS3DH.c
APPSRC += ProcessEv_UART.c

### Target type
# アプリケーション(.bin)をビルドするか、ライブラリ(.a)にするか指定します。
# - ライブラリをリンクする場合は、ADDITIONAL_LIBS に定義を加えてください。
#
TARGET_TYPE=bin # for building final application
#TARGET_TYPE=a  # for building library

### ToCoNet debug build
# ToCoNet のスタックデバッグ出力を使用する場合 1 を指定します。
# - ターゲットファイル名に _TDBG が追加されます。 
#
TOCONET_DEBUG ?= 0

##########################################################################
### Basic definitions (do not edit here)
# コンパイルオプションやディレクトリなどの値を設定します。
include ../../../../MkFiles/toconet.mk
##########################################################################

### CPP Definition, C flags
# 必要に応じてプリプロセッサ定義などを追加します。
# - 最適化オプションを含め多くのオプションは事前定義されているため
#   通常は指定不要です。
#
#CFLAGS += -DDUMMY
CFLAGS += -DENDDEVICE_INPUT

### Build Options
# プラットフォーム別のビルドをしたり、デバッグ用のビルドのファイル名を変更
# したいような場合の設定。
# - OBJDIR_SUFF は objs_$(JENNIC_CHIP) に追加されます。
# - TARGET_SUFF は生成ファイル名に追加されます。
#
# 以下の例では、make DEBUGOPT=1 とした時に、CPP の DEBUG を定義し、ファイル
# 名に _DBG を追加します。
#ifeq ($(DEBUGOPT),1)
#  CFLAGS += -DDEBUG
#  OBJDIR_SUFF += _Debug
#  TARGET_SUFF += _Debug
#endif 

### Additional Src/Include Path
# 下記に指定したディレクトリがソース検索パス、インクルード検索パスに設定
# されます。Makefile のあるディレクトリからの相対パスを指定します。
#
APP_COMMON_SRC_DIR_ADD1 = ../../Common/Sensor  
#APP_COMMON_SRC_DIR_ADD2 = 
#APP_COMMON_SRC_DIR_ADD3 = 
#APP_COMMON_SRC_DIR_ADD4 =
#
# インクルードディレクトリのみを追加指定したい場合は、-I オプションとして
# INCFLAGS に追加します。
#
#INCFLAGS += -I../dummy 

### Additional user objects (.a,.o)
# 追加でリンクしたいライブラリ、オブジェクトファイルを指定します。
# 
#ADDITIONAL_LIBS += ../../Common/libDummy_$(JENNIC_CHIP).a

### Additional library 
# コンパイラ付属ライブラリ (math, spp など) を指定します。
# ※ 各ライブラリの動作保証はありません。
#
#LDLIBS += m

### Additional linker options 
# リンカへの追加オプションです。
# - 必要なオプションは事前定義されているため通常は指定不要です。
# 
#LDFLAGS += -u dummy

#########################################################################
### Include rules (do not edit here)
# コンパイルルールが定義されます。
include ../../../../MkFiles/rules.mk
#########################################################################
