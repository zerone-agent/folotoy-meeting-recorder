#include "meeting_delete_workflow.h"
#include <assert.h>
#include <string.h>

typedef struct {uint8_t data[2][MEETING_DELETION_SIZE];bool present[2],fail_write,fail_proof,local_exists;int remote,removals,cancels;} fixture;
static int read_slot(void *ctx,unsigned slot,uint8_t *out){fixture *f=ctx;if(!f->present[slot])return 1;memcpy(out,f->data[slot],MEETING_DELETION_SIZE);return 0;}
static bool write_slot(void *ctx,unsigned slot,const uint8_t *data){fixture *f=ctx;if(f->fail_write)return false;memcpy(f->data[slot],data,MEETING_DELETION_SIZE);f->present[slot]=true;return true;}
static bool proof(void *ctx,const meeting_deletion *intent){fixture *f=ctx;assert(intent->file_id==42);return !f->fail_proof;}
static bool remove_local(void *ctx,const meeting_deletion *intent){fixture *f=ctx;assert(intent->file_id==42);f->removals++;f->local_exists=false;return true;}
static int cancel_remote(void *ctx,const meeting_deletion *intent){fixture *f=ctx;assert(intent->meeting_id[0]==1);f->cancels++;return f->remote;}
int main(void){
 fixture f={.local_exists=true};meeting_deletion_io storage={&f,read_slot,write_slot};meeting_delete_workflow_io io={&f,proof,remove_local,cancel_remote};
 meeting_deletion intent={.generation=1,.file_id=42,.meeting_id={1},.operation_id={2}},state;unsigned slot;
 assert(meeting_delete_resume(&storage,&io)==MEETING_DELETE_ERROR&&f.removals==0);
 assert(meeting_deletion_commit(&storage,&intent));
 f.fail_proof=true;assert(meeting_delete_resume(&storage,&io)==MEETING_DELETE_ERROR&&f.local_exists&&f.removals==0);f.fail_proof=false;
 f.fail_write=true;assert(meeting_delete_resume(&storage,&io)==MEETING_DELETE_ERROR&&!f.local_exists&&f.cancels==0);
 f.fail_write=false;assert(meeting_delete_resume(&storage,&io)==MEETING_DELETE_PENDING&&f.removals==2&&f.cancels==1);
 assert(meeting_deletion_load(&storage,&state,&slot)==MEETING_DELETION_OK&&state.local_done&&state.remote==MEETING_DELETE_REMOTE_PENDING);
 f.fail_proof=true;assert(meeting_delete_resume(&storage,&io)==MEETING_DELETE_ERROR&&f.cancels==1);f.fail_proof=false;
 f.remote=2;f.fail_write=true;assert(meeting_delete_resume(&storage,&io)==MEETING_DELETE_ERROR&&f.cancels==2);
 assert(meeting_deletion_load(&storage,&state,&slot)==MEETING_DELETION_OK&&state.remote==MEETING_DELETE_REMOTE_PENDING);
 f.fail_write=false;assert(meeting_delete_resume(&storage,&io)==MEETING_DELETE_DONE&&f.cancels==3);
 assert(meeting_deletion_load(&storage,&state,&slot)==MEETING_DELETION_OK&&state.remote==MEETING_DELETE_REMOTE_COMMITTED);
 assert(meeting_delete_resume(&storage,&io)==MEETING_DELETE_DONE&&f.removals==2&&f.cancels==3);
 return 0;
}
