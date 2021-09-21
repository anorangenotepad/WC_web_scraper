#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

xmlDocPtr
parseDoc(char *docname, char *uri)
{

  xmlDocPtr doc;
  xmlNodePtr cur;
  xmlNodePtr newnode;
  xmlAttrPtr newattr;

  doc = xmlParseFile(docname);

  if (doc == NULL) {
    fprintf(stderr, "ERR: Document not parsed sucessfully\n");
    return (NULL);
  }

  cur = xmlDocGetRootElement(doc);

  if (cur == NULL) {
    fprintf(stderr, "ERR: Empty document\n");
    xmlFreeDoc(doc);
    return (NULL);
  }

  if (xmlStrcmp(cur->name, (const xmlChar*) "Session")) {
    fprintf(stderr, "ERR: Document of the wrong type, root node != Session\n");
    xmlFreeDoc(doc);
    return(NULL);
  }

  newnode = xmlNewTextChild(cur, NULL, "Info", NULL);


  char attribute_array[19][200] = {

    "Creator",
    "RecordingAgency",
    "Title",
    "Description",
    "Revision",
    "MultimediaType",
    "Language",
    "Keywords",
    "Publisher",
    "Source",
    "Artist",
    "SourceDate",
    "SourceRights",
    "Producer",
    "Identifier",
    "ProducedDate",
    "Copyright",
    "SourcePublisher",
    "BaseFile"

  };


  for(int i = 0; i < 19; i++){
    newattr = xmlNewProp(newnode, attribute_array[i], uri);
  }

  return(doc);

}

int
main(int argc, char **argv){

  //char *docname;
  //char *uri;
  char *docname;
  char uri[100] = "m";
  xmlDocPtr doc;

  
  if (argc <= 1) {
    printf("Usage: %s desired_filename.nhsx\n", argv[0]);
    return(0);
  }

  docname = argv[1];
  //uri = argv[2];
  

  doc = parseDoc(docname, uri);
  if (doc != NULL){
    xmlSaveFormatFile(docname, doc, 1);
    xmlFreeDoc(doc);
  }
  
  return(1);
}
