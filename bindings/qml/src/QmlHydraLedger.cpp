/*
################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Please refer to the README for information about making permanent changes.  #
################################################################################
*/

#include "QmlHydraLedger.h"


///
//  Return ledger size, i.e. number of posts stored in the ledger.
size_t QmlHydraLedger::size () {
    return hydra_ledger_size (self);
};

///
//  Load the ledger data from disk, from the specified directory. Returns 
//  the number of posts loaded, or -1 if there was an error reading the   
//  directory.                                                            
int QmlHydraLedger::load () {
    return hydra_ledger_load (self);
};

///
//  Save a post via the ledger. This saves the post to disk, adds the post
//  to the ledger, and then destroys the post. Use in place of            
//  hydra_post_save to ensure post filenames are correctly generated.     
int QmlHydraLedger::store (QmlHydraPost *postP) {
    return hydra_ledger_store (self, &postP->self);
};

///
//  Return post at specified index; if the index does not refer to a valid
//  post, returns NULL.                                                   
QmlHydraPost *QmlHydraLedger::fetch (int index) {
    QmlHydraPost *retQ_ = new QmlHydraPost ();
    retQ_->self = hydra_ledger_fetch (self, index);
    return retQ_;
};

///
//  Lookup post in ledger and return post index (0 .. size - 1); if the post
//  does not exist, returns -1.                                             
int QmlHydraLedger::index (const QString &postId) {
    return hydra_ledger_index (self, postId.toUtf8().data());
};


QObject* QmlHydraLedger::qmlAttachedProperties(QObject* object) {
    return new QmlHydraLedgerAttached(object);
}


///
//  Self test of this class
void QmlHydraLedgerAttached::test (bool verbose) {
    hydra_ledger_test (verbose);
};

///
//  Create a new empty ledger instance.
QmlHydraLedger *QmlHydraLedgerAttached::construct () {
    QmlHydraLedger *qmlSelf = new QmlHydraLedger ();
    qmlSelf->self = hydra_ledger_new ();
    return qmlSelf;
};

///
//  Destroy the ledger
void QmlHydraLedgerAttached::destruct (QmlHydraLedger *qmlSelf) {
    hydra_ledger_destroy (&qmlSelf->self);
};

/*
################################################################################
#  THIS FILE IS 100% GENERATED BY ZPROJECT; DO NOT EDIT EXCEPT EXPERIMENTALLY  #
#  Please refer to the README for information about making permanent changes.  #
################################################################################
*/
