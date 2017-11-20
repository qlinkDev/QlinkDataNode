
AM_CFLAGS = -Wall \
	-Wundef \
        -Wstrict-prototypes \
        -Wno-trigraphs \
        -I$(WORKSPACE)/diag/include \
	$(QMI_CFLAGS)

ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -D__packed__= \
        -DIMAGE_APPS_PROC \
        -DFEATURE_Q_SINGLE_LINK \
        -DFEATURE_Q_NO_SELF_QPTR \
        -DFEATURE_LINUX \
        -DFEATURE_NATIVELINUX \
        -DFEATURE_DSM_DUP_ITEMS \
        -DFEATURE_LE_DIAG \
        -I$(WORKSPACE)/qcom-opensource/mm-audio/libalsa-intf

bin_PROGRAMS = qlinkdatanode
qlinkdatanode_CC = @CC@
pkgconfigdir = $(libdir)/pkgconfig
qlinkdatanode_SOURCES = main.c \
	qmi_parser_qlinkdatanode.c \
	qmi_receiver_qlinkdatanode.c \
	qmi_sender_qlinkdatanode.c \
	qmi_common_qlinkdatanode.c \
	at_qlinkdatanode.c \
	at_rsp_qlinkdatanode.c \
	common_qlinkdatanode.c \
	event_qlinkdatanode.c \
	queue_qlinkdatanode.c \
	protocol_qlinkdatanode.c \
	socket_qlinkdatanode.c \
	gpio_qlinkdatanode.c \
	msq_qlinkdatanode.c \
	sock_listener_qlinkdatanode.c \
	log_qlinkdatanode.c \
	qmi_uim_sender_qlinkdatanode.c \
	qmi_nas_sender_qlinkdatanode.c \
	qmi_dms_sender_qlinkdatanode.c \
	rst_qlinkdatanode.c \
	dh_ue.c \
	des.c

qlinkdatanode_LDFLAGS = -lpthread $(QMI_LIBS)
qlinkdatanode_LDADD = $(QMI_LIBS)
