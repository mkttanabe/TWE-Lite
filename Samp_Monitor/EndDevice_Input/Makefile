#########################################################################
#
# MODULE:   JN-AN-1061-Jenie-Application-Template
#
# DESCRIPTION: MakeFile
#
############################################################################
# 
# This software is owned by Jennic and/or its supplier and is protected
# under applicable copyright laws. All rights are reserved. We grant You,
# and any third parties, a license to use this software solely and
# exclusively on Jennic products. You, and any third parties must reproduce
# the copyright and warranty notice and any other legend of ownership on each
# copy or partial copy of the software.
# 
# THIS SOFTWARE IS PROVIDED "AS IS". JENNIC MAKES NO WARRANTIES, WHETHER
# EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE,
# ACCURACY OR LACK OF NEGLIGENCE. JENNIC SHALL NOT, IN ANY CIRCUMSTANCES,
# BE LIABLE FOR ANY DAMAGES, INCLUDING, BUT NOT LIMITED TO, SPECIAL,
# INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY REASON WHATSOEVER.
# 
# Copyright Jennic Ltd 2010. All rights reserved
#
#############################################################################
# Subversion variables
# $HeadURL: http://svn/apps/Application_Notes/JN-AN-1067-JenNet-Wireless-Sensor-Network/Tags/Release_1v5-Public/EndDevice/Makefile $
# $Revision: 5583 $
# $LastChangedBy: mlook $
# $LastChangedDate: 2010-03-17 13:33:14 +0000 (Wed, 17 Mar 2010) $
# $Id: Makefile 5583 2010-03-17 13:33:14Z mlook $ 
#
#############################################################################

DIRS	= Build

TARGETS: clean all

all: 
	-for d in $(DIRS); do (cd $$d; $(MAKE) $(MFLAGS) all ); done

clean: 
	-for d in $(DIRS); do (cd $$d; $(MAKE) $(MFLAGS) clean ); done

