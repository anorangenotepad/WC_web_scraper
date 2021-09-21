/*
 *SOURCE SITES

  https://rosettacode.org/wiki/Web_scraping
  https://github.com/curl/curl/blob/master/docs/examples/crawler.c
  http://www.xmlsoft.org/tutorial/apd.html
  https://www.codingame.com/playgrounds/14213/how-to-play-with-strings-in-c/array-of-c-string
  https://www.edureka.co/blog/linked-list-in-c/
  https://gist.github.com/ghedo/963382/815c98d1ba0eda1b486eb9d80d9a91a81d995283
  https://www.geeksforgeeks.org/write-a-function-to-get-nth-node-in-a-linked-list/
  https://www.geeksforgeeks.org/linked-list-set-2-inserting-a-node/
  https://www.geeksforgeeks.org/write-a-function-to-get-nth-node-in-a-linked-list/
  https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way

*/

/* NOTES
 *
 *  The way things are set up, the following functions point to (0 offset) locations
 *  in the linked list to format the data.
 *
 * format_author_name();
 * format_dates_publisher();
 *
 * THEY DONT USE GLOBAL VARIABLES, SO IF YOU ADD SOMETHING ELSE THE POSTIONS WILL CHANGE!
 * (this is also true for elments in arrays and stuff, but most of those have corresponding
 *  global variables now)
 *
 *  Currently, this program has the follwing features
 *  1) creates a linked list to store metadata
 *  2) allows user input of id, produced date, and narrator name
 *  3) gets supplemental information from a designated world cat site
 *  4) FILLS IN UNAVAILABLE VALUES WITH "no data"
 *  5) traverses the linked list to format some of the values (i.e. author name)
 *  6) writes the values from the linked list to the specified hinden file
 *
 *  NOTES FOR FUTURE DEVELOPMENT:
 *
 *  right now, the loop is turned off, so it will only get the first (lcsh) keyword
 *  (line 429 in html_xpath_handler())
 *  you could turn it back on, then concat() the values from the last 3 fields in the linked
 *  list and overwrite the keyword field with the new value
 *
 *
 */

//compile with: gcc -o c_web_scrape_html c_web_scrape_html.c -lcurl $(xml2-config --cflags --libs)

#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>

//ctype.h includes isspace() function
#include <ctype.h>

#include <libxml/xpath.h>
#include <libxml/parser.h>
//using parser.h will not work for html parsing
//you want this one for html
#include <libxml/HTMLparser.h>


#define BUFSIZE 100000

#define NUMBER_OF_QUERIES 6
#define NUMBER_OF_HINDEN_LOCATIONS 16 
//#define RESULT_METADATA_LINES 25
#define USER_INPUT_QUESTIONS 2
#define NUMBER_OF_STABLE_ARRAY_ITEMS 6
#define MAX_ARRAY_STRING_LEN 500 
#define MAX_METADATA_ELEMENTS 9
#define MAX_METADATA_STRING_LEN 3000

//global variable(s)
size_t lr = 0;

char hb_file_path[1000];
char hb_template [200] = "hb_tester.nhsx";

char dbc_prefix [200] = "xx-xxx-";


//struct, but this is for linked list...
struct node
{
  char item [MAX_METADATA_STRING_LEN];
  struct node *next;
};
struct node *start = NULL;

//function declarations (should probably use .h files
//and break into separate files...
//maybe in future revision

//get website and build queries to find necessary info
//from page (title, author, etc.)
int curl_site(char *target_website);
size_t filterit(void *ptr, size_t size, size_t nmemb, void *stream);
int site_xpath_query_builder(char *site_buffer);

int xpath_html_handler(char *buffer, char *xpath_string);

char* format_strings(char *target_string);
void metadata_linked_list(char *metadata);
int input_list_metadata(void);
int stable_list_metadata(void);
void list_metadata_insert(char *new_data);
int site_hinden_xpath_builder(void);

//acutally get the info to build the initial linked list
//then, write the values from the linked list to the hinden file
//and save it as a new file
int format_author_name();
int format_dates_publisher();
char* lowercase_convert(char *target_string);
char* trim_spaces(char *target_string);
char* replace_blank_entries(char *target_string);


int xpath_hinden_overwrite(char *h_xpath, char *l_list);
xmlDocPtr getdoc(char *docname);
xmlXPathObjectPtr getnodeset(xmlDocPtr doc, xmlChar *xpath);
void save_to_file(xmlDoc *doc, char *save_file_name);



void display_linked_list();


int
main(int argc, char **argv)
{

  char target_website[400];

  if (argc < 3)
  {
    printf("ERR: usage is: %s www.desired_worldcat_site.com desired_filename.nhsx\n", argv[0]);
    return -1;
  }

  strcpy(target_website, argv[1]);
  strcpy(hb_file_path, argv[2]);

  //this has to be up here
  //you are starting from the begining and building the list by adding 
  //elements after the first one
  input_list_metadata();
  stable_list_metadata();
  curl_site(target_website);
  site_hinden_xpath_builder();
  format_author_name();
  format_dates_publisher();


  //display_linked_list();

  return 0;
}

int
input_list_metadata(void)
{

  char pass_to_list [200];
  char uid_string [200];

  char question_array[USER_INPUT_QUESTIONS][MAX_ARRAY_STRING_LEN] = {
                               "narrator (last, first): ",
                               "produced date (YYYY-MM-DD): "
                             };


  printf("base filename (i.e. xxx12345)\n");
  fgets(pass_to_list, sizeof pass_to_list, stdin);
  pass_to_list[strcspn(pass_to_list, "\n")] = 0;
  
  lowercase_convert(pass_to_list);
  metadata_linked_list(pass_to_list);

  snprintf(uid_string, sizeof uid_string, "%s%s", dbc_prefix, pass_to_list);
  metadata_linked_list(uid_string);




  for(int p = 0; p < USER_INPUT_QUESTIONS; p++){

    printf("%s\n", question_array[p]);
    fgets(pass_to_list, sizeof pass_to_list, stdin);
    pass_to_list[strcspn(pass_to_list, "\n")] = 0;
    metadata_linked_list(pass_to_list);

  }


  return 0;
}

char*
lowercase_convert(char *target_string)
{
  
  unsigned long int c;

  for(c = 0; c <= strlen(target_string); c++){
    if(target_string[c] >= 65 && target_string[c] <= 90)
      target_string[c] = target_string[c]+32;
  }

  return target_string;
}


int
stable_list_metadata(void)
{


  char stable_array[NUMBER_OF_STABLE_ARRAY_ITEMS][MAX_ARRAY_STRING_LEN] = {
                               "audioNCX",
                               "en-us",
                               "Further reproduction or distribution in other than a specialized format is prohibited",
                               "Library Name", 
                               "Lib Code",
                               "Parent Organization"
                             };

 
  for(int p = 0; p < NUMBER_OF_STABLE_ARRAY_ITEMS; p++){

    
    metadata_linked_list(stable_array[p]);

  }


  return 0;
}

int 
curl_site(char *target_website)
{
  CURL *curlHandle;
  char site_buffer[BUFSIZE];

  curlHandle = curl_easy_init();
  curl_easy_setopt(curlHandle, CURLOPT_URL, target_website);
  curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, filterit);
  curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, site_buffer);
  int success = curl_easy_perform(curlHandle);
  curl_easy_cleanup(curlHandle);

  //the entire html gets saved to buffer
  //you can see this by printing buffer
  //printf("%s\n", site_buffer);

  site_xpath_query_builder(site_buffer);

  return 0;
}


size_t 
filterit(void *ptr, size_t size, size_t nmemb, void *stream)
{
  if ( (lr + size*nmemb) > BUFSIZE ) return BUFSIZE;
  memcpy(stream+lr, ptr, size*nmemb);
  lr += size*nmemb;
  return size*nmemb;
}

int
site_xpath_query_builder(char *site_buffer)
{

  //this seems like the cleanest query for finding just info I want on worldcat

  
  char xpath_array[NUMBER_OF_QUERIES][MAX_ARRAY_STRING_LEN] = {
                               "//*[@class='title']", 
                               "//*[@id='bib-author-cell']//*[@title='Search for more by this author'][1]",
                               "//*[@id='bib-publisher-cell']",
                               "//*[@id='summary' or @class='nielsen-review']",
                               "//td[contains(text(),'978')]",
                               "//*[@title='Search for more with this topic']"
                             };

  

  //this kind of works for schema.org ...but not very well
  //char test_xpath[100] = "//*[@property]"; 

  //char test_xpath[100]="//td[contains(text(),'978')]";
   
 
  for(int p = 0; p < NUMBER_OF_QUERIES; p++){

    xpath_html_handler(site_buffer, xpath_array[p]);
  }


  return 0;
}


int
xpath_html_handler(char *site_buffer, char *xpath_string)
{

  char *docname;
  
  //in normal xml docs can use xmlDocPtr, BUT
  //for html need htmlDocPtr
  htmlDocPtr doc;


  xmlNodePtr pNode = NULL;
  xmlNodeSetPtr pNodeSet = NULL;

  xmlChar *xpath;
  xmlChar xpath_query[300];
  snprintf(xpath_query, sizeof xpath_query, "%s", xpath_string);
  xpath = (xmlChar*) xpath_query;

  xmlNodeSetPtr nodeset;
  xmlXPathObjectPtr result;
  int i;
  xmlChar *keyword;
  char blank_space[20] = "no data";

  
  char *HTMLBUFF = site_buffer;
  int opts = HTML_PARSE_NOBLANKS | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET;
  doc = htmlReadMemory(HTMLBUFF, strlen(HTMLBUFF), "url", NULL, opts);



  result = getnodeset(doc, xpath);

  //this is error handling and we need because some records do not contain all the 
  //info we are looking for
  //if no error handling, values will be written to the wrong fields
  //this writes "no data" to fields with no info

  if (result == NULL)
  {
    //printf("result was NULL\n");
    metadata_linked_list(blank_space);

    
  }

  

  if(result != NULL)
  {

    nodeset = result->nodesetval;
    //cannot take loop out (but i did for now)
    //more than one result for lcsh sub headings
    //for(i = 0; i < nodeset->nodeNr; i++){

      keyword = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1);
      printf("-> %s\n", keyword);



      replace_blank_entries(keyword);
      format_strings(keyword);
      trim_spaces(keyword);

      
      metadata_linked_list(keyword);


      xmlFree(keyword);
    //}

    xmlXPathFreeObject(result);
  }

  xmlFreeDoc(doc);
  xmlCleanupParser();

 
  return(1);




}

int
site_hinden_xpath_builder(void)
{

  //builds xpaths for identifying locations of metadata elements
  //in hinden file

  
  char hinden_xpath_array[NUMBER_OF_HINDEN_LOCATIONS][MAX_ARRAY_STRING_LEN] = {
                               
                               "//Info/@BaseFile",
                               "//Info/@Identifier",
                               "//Info/@Artist",
                               "//Info/@ProducedDate",
                               "//Info/@MultimediaType",
                               "//Info/@Language",
                               "//Info/@Copyright",
                               "//Info/@RecordingAgency",
                               "//Info/@Producer",
                               "//Info/@Publisher",
                               "//Info/@Title", 
                               "//Info/@Creator",
                               "//Info/@SourcePublisher",
                               "//Info/@Description",
                               "//Info/@Source",
                               "//Info/@Keywords"
                             };

  struct node *ptr;
  ptr = start;
 
  for(int p = 0; p < NUMBER_OF_HINDEN_LOCATIONS; p++){

    xpath_hinden_overwrite(hinden_xpath_array[p], ptr->item);
    ptr = ptr->next;
  }


  return 0;
}

int
xpath_hinden_overwrite(char *h_xpath, char *l_list)
{

  char *docname;
  xmlDocPtr doc;

  


  xmlNodePtr pNode = NULL;
  xmlNodeSetPtr pNodeSet = NULL;

  xmlChar *xpath;

  //might want to make a loop here, so each time it runs through
  //another path from the array get assinged to xpath string

  xmlChar xpath_query[300];
  

  snprintf(xpath_query, sizeof xpath_query, "%s", h_xpath);
  xpath = (xmlChar*) xpath_query;

  xmlNodeSetPtr nodeset;
  xmlXPathObjectPtr result;
  int i;
  xmlChar *keyword;
  
  docname = hb_file_path;
  doc = getdoc(docname);

  result = getnodeset(doc, xpath);

  char new_content_string[MAX_METADATA_STRING_LEN];
  strcpy(new_content_string, l_list);



  if(result){

    nodeset = result->nodesetval;
    //cannot take loop out
    //more than one result for lcsh sub headings
    for(i = 0; i < nodeset->nodeNr; i++){

      keyword = xmlNodeListGetString(doc, nodeset->nodeTab[i]->xmlChildrenNode, 1);
      pNode = nodeset->nodeTab[i];

      printf("-> %s\n", new_content_string);


      xmlNodeSetContent(pNode, new_content_string);
      save_to_file(doc, hb_file_path);



      xmlFree(keyword);
    }

    xmlXPathFreeObject(result);
  }

  xmlFreeDoc(doc);
  xmlCleanupParser();

 
  return(1);

}

xmlDocPtr
getdoc (char *docname)
{

  xmlDocPtr doc;
  doc = xmlParseFile(docname);

  if (doc == NULL){

    fprintf(stderr, "ERR: doc not parse successfully\n");
    return NULL;
  }

  return doc;
}

xmlXPathObjectPtr
getnodeset (xmlDocPtr doc, xmlChar *xpath)
{

  xmlXPathContextPtr context;
  xmlXPathObjectPtr result;


  context = xmlXPathNewContext(doc);
  if (context == NULL) {
    printf("ERR: xmlXPathNewContext\n");
    return NULL;
  }

  result = xmlXPathEvalExpression(xpath, context);
  xmlXPathFreeContext(context);

  if (result == NULL) {
    printf("ERR: xmlXPathEvalExpression\n");
    return NULL;
  }

  if(xmlXPathNodeSetIsEmpty(result->nodesetval)){
    xmlXPathFreeObject(result);
    printf("no result (i.e. target node not found)\n");
    return NULL;
   

  }
  
  return result;
}

void
save_to_file(xmlDoc *doc, char *save_file_name)
{

  xmlChar *xmlbuff;
  int buffersize;
  FILE *fp;

  xmlDocDumpFormatMemory(doc, &xmlbuff, &buffersize, 1);

  fp = fopen(save_file_name, "w+");
  fputs(xmlbuff, fp);
  fclose(fp);

}

char*
format_strings(char *target_string)
{
  char *finder = target_string;

  while (*finder)
  {
    if ((*finder == '\n') || (*finder == '"') || (*finder == '[') || (*finder == ']'))
      *finder = ' ';
    finder++;
  }

return target_string;
}


void
metadata_linked_list(char *metadata)
{

  struct node *temp, *ptr;
  temp = (struct node*) malloc (sizeof(struct node));
  
  if (temp == NULL)
  {
    printf("ERR: out of mem space\n");
    exit(0);
  }

  strcpy(temp->item, metadata);
  temp->next = NULL;
  if (start == NULL)
  {
    start = temp;
  }
  else
  {
    ptr = start;
    while (ptr->next != NULL)
    {
      ptr = ptr->next;
    }
    ptr->next = temp;
  }
  
}

int
format_author_name()
{
  
  struct node* current = start;
  char author_name[200];
  char formatted_author_name[200];
  const char delim[2] = " ";
  char *token;

  char fName[100];
  char lName[100];

  int counter = 0;
  while (current != NULL){
    if (counter == 11)
      strcpy(author_name, current->item);
    counter++;
    current = current->next;
  }
    
  strcpy(fName, strtok(author_name, delim));
  strcpy(lName, strtok(NULL, delim));

  snprintf(formatted_author_name, sizeof formatted_author_name, "%s, %s", lName, fName);

  xpath_hinden_overwrite("//Info/@Creator", formatted_author_name);

  return 0;

}

int
format_dates_publisher()
{
  
  struct node* current = start;
  char publisher_string[500];
  char publisher_and_date[300];
  char formatted_publisher_name[400];
  char formatted_date[100];
  const char delim[2] = ":";
  const char delim2[2] = ",";
  char *token, *token2;
   
  int counter = 0;
  while (current != NULL){
    if (counter == 12)
      strcpy(publisher_string, current->item);
    counter++;
    current = current->next;
  }
   
  strtok(publisher_string, delim);
  strcpy(formatted_publisher_name, strtok(NULL, delim));

  strtok(formatted_publisher_name, delim2);
  strcpy(formatted_date, strtok(NULL, delim2));

  trim_spaces(formatted_date);

  xpath_hinden_overwrite("//Info/@SourceDate", formatted_date);
  xpath_hinden_overwrite("//Info/@SourceRights", formatted_date);

  /* This is very odd, but if you put this (publisher name)  before the ones that filter off the 
   * date, it will leave the date on the end...
   * doing it this way was just a trial, but it appears to have worked as you only get the publisher!
   */

  trim_spaces(formatted_publisher_name);
  xpath_hinden_overwrite("//Info/@SourcePublisher", formatted_publisher_name);


  return 0;

}

char* 
trim_spaces(char *target_string)
{
  int i;
  int begin = 0;
  int end = strlen(target_string) - 1;

  while (isspace((unsigned char) target_string[begin]))
    begin++;

  while ((end >= begin) && isspace((unsigned char) target_string[end]))
    end--;

  // Shift all characters back to the start of the string array
  for (i = begin; i <= end; i++)
    target_string[i - begin] = target_string[i];

  target_string[i - begin] = '\0';

  return target_string;



}

char*
replace_blank_entries(char *target_string)
{

  
  char blank_space[20] = "blank";

  if (strlen(target_string) == 0)
  {
    strcpy(target_string, blank_space);
    return target_string;
  }
  else
  {

    return target_string;

  }


  printf("did it\n");

}


/*
void
display_linked_list()
{

  struct node *ptr;
  if (start == NULL)
  {
    printf("ERR: list is empty\n");
    return;
  }
  else
  {
    ptr = start;
    //while (ptr != NULL)
    for (int q = 0; q < MAX_METADATA_ELEMENTS; q++)
    {
      printf("%s\n", ptr->item);
      ptr = ptr->next;
    }
  }
}
*/


