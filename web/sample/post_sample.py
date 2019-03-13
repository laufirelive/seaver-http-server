import os
import cgi, cgitb 

form = cgi.FieldStorage() 

username = str(form.getvalue('user'))
password = str(form.getvalue('passwd'))

context = "<!DOCTYPE html>"
context += "<html>"
context += "<head>"
context += "<meta charset=\"utf-8\">"
context += "<title>Post Sample</title>"
context += "</head>"
context += "<body style=\"text-align: center\">"
context += "<h1>Result</h1>"
context += "<hr>"
context += "<h2> Welcome " + username + "</h2>"
context += "<h2> " + password + " is your password, remember it well.</h2>"
context += "</body>"

print("Content-type:text/html")
print("Content-length:" + str(len(context)))
print("")
print(context)
