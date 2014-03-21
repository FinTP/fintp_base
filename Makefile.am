ACLOCAL_AMFLAGS = -I ../fintp_utils/m4

lib_LTLIBRARIES = libfintp_base.la

AM_CPPFLAGS = $(CFLAGS_INCL)	\
@CPPFLAGS@	\
@PTHREAD_CFLAGS@	\
@OPENSSL_INCLUDES@  \
@BOOST_CPPFLAGS@	\
@LIBCURL_CPPFLAGS@ 	\
@XERCES_CPPFLAGS@	\
@XALAN_CPPFLAGS@	\
@ZIP_CPPFLAGS@	\
-I$(FINTP_HOME)/fintp_utils/src	\
-I$(FINTP_HOME)/fintp_log/src	\
-I$(FINTP_HOME)/fintp_transport/src	\
-I$(FINTP_HOME)/fintp_udal/src	\
-I$(FINTP_HOME)/fintp_base/src	\
-I$(FINTP_HOME)/fintp_ws/src

AUTOMAKE_OPTIONS = foreign

libfintp_base_la_LDFLAGS = @LIBS@ @LDFLAGS@	\
@PTHREAD_LIBS@	\
@OPENSSL_LDFLAGS@ @OPENSSL_LIBS@  \
@BOOST_LDFLAGS@ @BOOST_REGEX_LIB@ -lboost_regex -lboost_filesystem \
@LIBCURL@	\
@XERCES_LDFLAGS@ @XALAN_LDFLAGS@ -lxerces-c -lxalan-c	\
@ZIP_LDFLAGS@ @ZIP_LIBS@	\
-L$(FINTP_HOME)/fintp_utils/.libs -lfintp_utils	\
-L$(FINTP_HOME)/fintp_log/.libs -lfintp_log	\
-L$(FINTP_HOME)/fintp_transport/.libs -lfintp_transport	\
-L$(FINTP_HOME)/fintp_udal/.libs -lfintp_udal

SUBDIRS = .

CFLAGS_INCL = -DUNIX -DDEBUG_ENABLED -DXALAN_1_9 -DZIPARCHIVE

libfintp_base_la_SOURCES = src/AbstractFilter.cpp	\
src/AbstractFilter.h	\
src/AbstractWatcher.cpp	\
src/AbstractWatcher.h	\
src/AppSettings.cpp	\
src/AppSettings.h	\
src/Base64Filter.cpp	\
src/Base64Filter.h	\
src/BatchManager/BatchConfig.cpp	\
src/BatchManager/BatchConfig.h	\
src/BatchManager/BatchDbOps.cpp	\
src/BatchManager/BatchDbOps.h	\
src/BatchManager/BatchItem.cpp	\
src/BatchManager/BatchItem.h	\
src/BatchManager/BatchItemEval.cpp	\
src/BatchManager/BatchItemEval.h	\
src/BatchManager/BatchManager.cpp	\
src/BatchManager/BatchManager.h	\
src/BatchManager/BatchResolution.cpp	\
src/BatchManager/BatchResolution.h	\
src/BatchManager/BatchStorageBase.cpp	\
src/BatchManager/BatchStorageBase.h	\
src/BatchManager/Evaluators/BatchItemACHEval.cpp	\
src/BatchManager/Evaluators/BatchItemACHEval.h	\
src/BatchManager/Evaluators/BatchItemGSRSEval.cpp	\
src/BatchManager/Evaluators/BatchItemGSRSEval.h	\
src/BatchManager/Evaluators/BatchItemMT104Eval.cpp	\
src/BatchManager/Evaluators/BatchItemMT104Eval.h	\
src/BatchManager/Evaluators/BatchItemMT940Eval.cpp	\
src/BatchManager/Evaluators/BatchItemMT940Eval.h	\
src/BatchManager/Evaluators/BatchItemMT950Eval.cpp	\
src/BatchManager/Evaluators/BatchItemMT950Eval.h	\
src/BatchManager/Storages/BatchFlatfileStorage.cpp	\
src/BatchManager/Storages/BatchFlatfileStorage.h	\
src/BatchManager/Storages/BatchMQStorage.cpp	\
src/BatchManager/Storages/BatchMQStorage.h	\
src/BatchManager/Storages/BatchXMLfileStorage.cpp	\
src/BatchManager/Storages/BatchXMLfileStorage.h	\
src/BatchManager/Storages/BatchZipArchiveStorage.cpp	\
src/BatchManager/Storages/BatchZipArchiveStorage.h	\
src/Currency.cpp	\
src/Currency.h	\
src/DB/DbDad.cpp	\
src/DB/DbDad.h	\
src/DB/DbWatcher.cpp	\
src/DB/DbWatcher.h	\
src/FS/FsWatcher.cpp	\
src/FS/FsWatcher.h	\
src/FilterChain.cpp	\
src/FilterChain.h	\
src/InstrumentedObject.cpp	\
src/InstrumentedObject.h	\
src/MQ/MqFilter.cpp	\
src/MQ/MqFilter.h	\
src/MQ/MqWatcher.cpp	\
src/MQ/MqWatcher.h	\
src/RoutingAggregationCode.cpp	\
src/RoutingAggregationCode.h	\
src/RoutingKeyword.cpp	\
src/RoutingKeyword.h	\
src/RoutingMessageEvaluator.cpp	\
src/RoutingMessageEvaluator.h	\
src/SSL/HMAC.cpp	\
src/SSL/HMAC.h	\
src/SSL/MD5.cpp	\
src/SSL/MD5.h	\
src/SSL/P7MFilter.cpp	\
src/SSL/P7MFilter.h	\
src/SSL/SSLFilter.cpp	\
src/SSL/SSLFilter.h	\
src/Swift/SAAFilter.cpp	\
src/Swif/SAAFilter.h	\
src/Swift/SwiftFormatFilter.cpp	\
src/Swift/SwiftFormatFilter.h	\
src/Template/TemplateParser.cpp	\
src/Template/TemplateParser.h	\
src/Template/TemplateTransformFilter.cpp	\
src/Template/TemplateTransformFilter.h	\
src/Transactions/AbstractStatePersistence.cpp	\
src/Transactions/AbstractStatePersistence.h	\
src/Transactions/FileMetadataStatePersist.cpp	\
src/Transactions/FileMetadataStatePersist.h	\
src/Transactions/MemoryStatePersist.cpp	\
src/Transactions/MemoryStatePersist.h	\
src/Version.h	\
src/VersionInfo.cpp	\
src/XPathHelper.cpp	\
src/XPathHelper.h	\
src/XPathHelper.new	\
src/XSD/XSDFilter.cpp	\
src/XSD/XSDFilter.h	\
src/XSD/XSDValidationException.cpp	\
src/XSD/XSDValidationException.h	\
src/XSLT/ExtensionAscii.cpp	\
src/XSLT/ExtensionAscii.h	\
src/XSLT/ExtensionBase64.cpp	\
src/XSLT/ExtensionBase64.h	\
src/XSLT/ExtensionHash.cpp	\
src/XSLT/ExtensionHash.h	\
src/XSLT/ExtensionLookup.cpp	\
src/XSLT/ExtensionLookup.h	\
src/XSLT/ExtensionNewLine.cpp	\
src/XSLT/ExtensionNewLine.h	\
src/XSLT/ExtensionRegex.cpp	\
src/XSLT/ExtensionRegex.h	\
src/XSLT/ExtensionTemplate.cpp	\
src/XSLT/ExtensionTemplate.h	\
src/XSLT/ExtensionTime.cpp	\
src/XSLT/ExtensionTime.h	\
src/XSLT/ExtensionUrl.cpp	\
src/XSLT/ExtensionUrl.h	\
src/XSLT/XSLTFilter.cpp	\
src/XSLT/XSLTFilter.h	\
src/Zip/ZipFilter.cpp	\
src/Zip/ZipFilter.h
