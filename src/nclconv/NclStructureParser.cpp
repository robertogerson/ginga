/* Copyright (C) 2006-2017 PUC-Rio/Laboratorio TeleMidia

This file is part of Ginga (Ginga-NCL).

Ginga is free software: you can redistribute it and/or modify it
under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Ginga is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
License for more details.

You should have received a copy of the GNU General Public License
along with Ginga.  If not, see <http://www.gnu.org/licenses/>.  */

#include "ginga.h"
#include "NclStructureParser.h"

#include "NclComponentsParser.h"

#include "NclParser.h"

GINGA_PRAGMA_DIAG_IGNORE (-Wsign-conversion)

GINGA_NCLCONV_BEGIN

NclStructureParser::NclStructureParser (NclParser *nclParser)
    : ModuleParser (nclParser)
{
}

ContextNode *
NclStructureParser::parseBody (DOMElement *parentElement)
{
  ContextNode *body = createBody (parentElement);
  g_assert_nonnull (body);

  DOMNodeList *elementNodeList = parentElement->getChildNodes ();

  int size = (int) elementNodeList->getLength ();
  for (int i = 0; i < size; i++)
    {
      DOMNode *node = elementNodeList->item (i);
      if (node->getNodeType () == DOMNode::ELEMENT_NODE)
        {
          DOMElement *element = (DOMElement *)node;
          string tagname = _nclParser->getTagname(element);
          if (XMLString::compareIString (tagname.c_str (), "media") == 0)
            {
              Node *media = _nclParser->getComponentsParser()
                      ->parseMedia (element);

              if (media)
                {
                  // add media to body
                  _nclParser->getComponentsParser ()
                          ->addMediaToContext (body, media);
                }
            }
          else if (XMLString::compareIString (tagname.c_str (), "context") == 0)
            {
              Node *child_context = _nclParser->getComponentsParser()
                      ->parseContext (element);

              if (child_context)
                {
                  // add context to body
                  _nclParser->getComponentsParser ()
                          ->addContextToContext (body, child_context);
                }
            }
          else if (XMLString::compareIString (tagname.c_str (), "switch") == 0)
            {
              Node *switch_node =
                      _nclParser->getPresentationControlParser ()
                        ->parseSwitch (element);

              if (switch_node)
                {
                  // add switch to body
                  _nclParser->getComponentsParser ()
                          ->addSwitchToContext (body, switch_node);
                }
            }
        }
    }

  for (int i = 0; i < size; i++)
    {
      DOMNode *node = elementNodeList->item (i);
      if (node->getNodeType () == DOMNode::ELEMENT_NODE)
        {
          DOMElement *element = (DOMElement *) node;
          string tagname = _nclParser->getTagname(element);
          if(XMLString::compareIString (tagname.c_str(), "property") == 0)
            {
              PropertyAnchor *prop = _nclParser->getInterfacesParser ()
                  ->parseProperty ((DOMElement *)node, body);

              if (prop)
                {
                  // add property to body
                  _nclParser->getComponentsParser ()
                      ->addPropertyToContext (body, prop);
                }
            }
        }
      }

  return body;
}

void
NclStructureParser::parseHead (DOMElement *head_element)
{
  NclDocument *nclDoc = getNclParser()->getNclDocument();
  g_assert_nonnull (nclDoc);

  for (DOMElement *child = head_element->getFirstElementChild();
       child != nullptr;
       child = child->getNextElementSibling())
    {
      string tagname = _nclParser->getTagname(child);
      if (XMLString::compareIString (tagname.c_str(), "importedDocumentBase")
          == 0)
        {
          _nclParser->getImportParser ()
              ->parseImportedDocumentBase (child);
        }
    }

  for (DOMElement *child = head_element->getFirstElementChild();
       child != nullptr;
       child = child->getNextElementSibling())
    {
      string tagname = _nclParser->getTagname(child);
      if( XMLString::compareIString (tagname.c_str(), "regionBase") == 0)
        {
          RegionBase *regionBase = _nclParser->getLayoutParser ()
              ->parseRegionBase (child);

          if (regionBase)
            {
              nclDoc->addRegionBase(regionBase);
            }
        }
    }

  for (DOMElement *child = head_element->getFirstElementChild();
       child != nullptr;
       child = child->getNextElementSibling())
    {
      string tagname = _nclParser->getTagname(child);
      if (XMLString::compareIString (tagname.c_str(), "ruleBase") == 0)
        {
          RuleBase *ruleBase = _nclParser->getPresentationControlParser ()
                    ->parseRuleBase (child);
          if (ruleBase)
            {
              nclDoc->setRuleBase (ruleBase);
              break;
            }
        }
    }

  for (DOMElement *child = head_element->getFirstElementChild();
       child != nullptr;
       child = child->getNextElementSibling())
    {
      string tagname = _nclParser->getTagname(child);
      if (XMLString::compareIString (tagname.c_str(), "transitionBase") == 0)
        {
          TransitionBase *transBase =
              _nclParser->getTransitionParser ()
                ->parseTransitionBase (child);
          if (transBase)
            {
              nclDoc->setTransitionBase (transBase);
              break;
            }
        }
    }

  for (DOMElement *child = head_element->getFirstElementChild();
       child != nullptr;
       child = child->getNextElementSibling())
    {
      string tagname = _nclParser->getTagname(child);
      if (XMLString::compareIString (tagname.c_str(), "descriptorBase") == 0)
        {
          DescriptorBase *descBase =
              _nclParser->getPresentationSpecificationParser ()
                ->parseDescriptorBase (child);
          if (descBase)
            {
              nclDoc->setDescriptorBase (descBase);
              break;
            }
        }
    }

  for (DOMElement *child = head_element->getFirstElementChild();
       child != nullptr;
       child = child->getNextElementSibling())
    {
      string tagname = _nclParser->getTagname(child);
      if (XMLString::compareIString (tagname.c_str(), "connectorBase") == 0)
        {
          ConnectorBase *connBase = _nclParser->getConnectorsParser ()
              ->parseConnectorBase (child, nclDoc);
          if (connBase)
            {
              nclDoc->setConnectorBase(connBase);
              break;
            }
        }
    }

  for (DOMElement *child = head_element->getFirstElementChild();
       child != nullptr;
       child = child->getNextElementSibling())
    {
      string tagname = _nclParser->getTagname(child);
      if (XMLString::compareIString (tagname.c_str(), "meta") == 0)
        {
          Meta *meta = _nclParser->getMetainformationParser ()
              ->parseMeta (child);
          if (meta)
            {
              nclDoc->addMetainformation (meta);
              break;
            }
        }
    }

  for (DOMElement *child = head_element->getFirstElementChild();
       child != nullptr;
       child = child->getNextElementSibling())
    {
      string tagname = _nclParser->getTagname(child);
      if (XMLString::compareIString (tagname.c_str(), "metadata") == 0)
        {
          Metadata *metadata = _nclParser->getMetainformationParser ()
              ->parseMetadata (child);
          if (metadata)
            {
              nclDoc->addMetadata (metadata);
              break;
            }
        }
    }
}

NclDocument *
NclStructureParser::parseNcl (DOMElement *parentElement)
{
  NclDocument* parentObject = createNcl (parentElement);
  g_assert_nonnull (parentObject);

  DOMNodeList *elementNodeList = parentElement->getChildNodes ();
  int size = (int) elementNodeList->getLength ();

  for (int i = 0; i < size; i++)
    {
      DOMNode *node = elementNodeList->item (i);
      if (node->getNodeType () == DOMNode::ELEMENT_NODE
          && XMLString::compareIString (((DOMElement *)node)->getTagName (),
                                        XMLString::transcode ("head"))
                 == 0)
        {
          parseHead ((DOMElement *)node);
        }
    }

  for (int i = 0; i < size; i++)
    {
      DOMNode *node = elementNodeList->item (i);
      if (node->getNodeType () == DOMNode::ELEMENT_NODE
          && XMLString::compareIString (((DOMElement *)node)->getTagName (),
                                        XMLString::transcode ("body"))
                 == 0)
        {
          ContextNode *body = parseBody ((DOMElement *)node);
          if (body)
            {
              posCompileBody ((DOMElement *)node, body);
              // addBodyToNcl (parentObject, body);
              break;
            }
        }
    }

  return parentObject;
}

ContextNode *
NclStructureParser::createBody (DOMElement *parentElement)
{
  // criar composicao a partir do elemento body do documento ncl
  // fazer uso do nome da composicao body que foi atribuido pelo
  // compilador
  NclDocument *document;
  ContextNode *context;

  document = getNclParser ()->getNclDocument ();
  if (!parentElement->hasAttribute (XMLString::transcode ("id")))
    {
      parentElement->setAttribute (
          XMLString::transcode ("id"),
          XMLString::transcode (document->getId ().c_str ()));

      context = (ContextNode *) _nclParser->getComponentsParser ()
                    ->createContext (parentElement);

      parentElement->removeAttribute (XMLString::transcode ("id"));
    }
  else
    {
      context = (ContextNode *)_nclParser->getComponentsParser ()
                    ->createContext (parentElement);
    }

  document->setBody (context);

  return context;
}

void
NclStructureParser::solveNodeReferences (CompositeNode *composition)
{
  NodeEntity *nodeEntity;
  Entity *referredNode;
  vector<Node *> *nodes;
  bool deleteNodes = false;

  if (composition->instanceOf ("SwitchNode"))
    {
      deleteNodes = true;
      nodes = _nclParser->getPresentationControlParser ()
                ->getSwitchConstituents ((SwitchNode *)composition);
    }
  else
    {
      nodes = composition->getNodes ();
    }

  if (nodes)
    {
      return;
    }

  for (Node *node : *nodes)
  {
    if (node != NULL)
      {
        if (node->instanceOf ("ReferNode"))
          {
            referredNode = ((ReferNode *)node)->getReferredEntity ();
            if (referredNode != NULL)
              {
                if (referredNode->instanceOf ("ReferredNode"))
                  {
                    nodeEntity = (NodeEntity *)(getNclParser ()->getNode (
                                                    referredNode->getId ()));

                    if (nodeEntity)
                      {
                        ((ReferNode *)node)
                            ->setReferredEntity (
                                nodeEntity->getDataEntity ());
                      }
                    else
                      {
                        syntax_error ("media: bad refer '%s'",
                                      referredNode->getId ().c_str ());
                      }
                  }
              }
          }
        else if (node->instanceOf ("CompositeNode"))
          {
            solveNodeReferences ((CompositeNode *)node);
          }
      }
  }

  if (deleteNodes)
    {
      delete nodes;
    }
}

void *
NclStructureParser::posCompileBody (DOMElement *parentElement,
                                    ContextNode *body)
{
  solveNodeReferences (body);

  return _nclParser->getComponentsParser ()
          ->posCompileContext (parentElement, body);
}

NclDocument*
NclStructureParser::createNcl (DOMElement *parentElement)
{
  string docName;
  NclDocument *document;

  if (parentElement->hasAttribute (XMLString::transcode ("id")))
    {
      docName = XMLString::transcode (
          parentElement->getAttribute (XMLString::transcode ("id")));
    }

  if (docName == "")
    docName = "ncl";

  document = new NclDocument (docName, _nclParser->getPath ());
  _nclParser->setNclDocument (document);

  return document;
}

GINGA_NCLCONV_END
