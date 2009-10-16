/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#ifndef DATAVARIANTEDITOR_H
#define DATAVARIANTEDITOR_H

#include "DataVariant.h"
#include "EnumWrapper.h"

#include <QtGui/QWidget>

#include <string>

class CustomColorButton;
class QDateTimeEdit;
class QDoubleValidator;
class QIntValidator;
class QLineEdit;
class QListWidget;
class QPushButton;
class QRadioButton;
class QStackedWidget;
class QTextEdit;
class SymbolTypeGrid;

class DataVariantEditorDelegate
{
public:
   enum TypeEnum { INTEGRAL, DOUBLE, BOOL, DATE_TIME, ENUMERATION, VECTOR, DYNAMIC_OBJECT, TEXT, COLOR, SYMBOL_TYPE };
   typedef EnumWrapper<TypeEnum> Type;

   DataVariantEditorDelegate() :
      mNeedBrowseButton(false)
   {
   }

   DataVariantEditorDelegate(std::string typeName, Type theType, bool needBrowseButton = false) :
      mType(theType),
      mTypeName(typeName),
      mNeedBrowseButton(needBrowseButton)
   {
   }

   Type getType() const
   {
      return mType;
   }

   void setType(Type newType)
   {
      mType = newType;
   }

   const std::string& getTypeName() const
   {
      return mTypeName;
   }

   void setTypeName(const std::string& newType)
   {
      mTypeName = newType;
   }

   bool getNeedBrowseButton() const
   {
      return mNeedBrowseButton;
   }

   void setNeedBrowseButton(bool newValue)
   {
      mNeedBrowseButton = newValue;
   }

   const std::vector<std::string>& getEnumValueStrings() const
   {
      return mEnumValues;
   }

   void setEnumValueStrings(const std::vector<std::string>& newValues)
   {
      mEnumValues = newValues;
   }
private:
   Type mType;
   std::string mTypeName;
   bool mNeedBrowseButton;
   std::vector<std::string> mEnumValues;
};

class DataVariantEditor : public QWidget
{
   Q_OBJECT

public:
   DataVariantEditor(QWidget* pParent = NULL, bool includeValueLabel = true);
   ~DataVariantEditor();

   void setValue(const DataVariant& value, bool useVariantCurrentValue = true);
   const DataVariant &getValue();

   static const std::vector<DataVariantEditorDelegate>& getDelegates();
   static DataVariantEditorDelegate getDelegate(const std::string& type);
   static bool hasDelegate(const std::string& type);

   virtual QSize sizeHint() const;

signals:
   void modified();

protected slots:
   void setStackWidget(std::string type);
   void browse();

private:
   QStackedWidget* mpStack;
   QLineEdit* mpValueLineEdit;
   QPushButton* mpBrowseButton;
   QListWidget* mpValueList;
   QTextEdit* mpValueTextEdit;
   QRadioButton* mpFalseRadio;
   QRadioButton* mpTrueRadio;
   QDateTimeEdit* mpDateTimeEdit;
   CustomColorButton* mpColorEdit;
   SymbolTypeGrid* mpSymbolTypeEdit;

   QIntValidator* mpIntValidator;
   QDoubleValidator* mpDoubleValidator;

   DataVariant mValue;
   static std::vector<DataVariantEditorDelegate> sDelegates;
};

#endif
