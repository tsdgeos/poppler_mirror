#include <QtTest/QtTest>

#include <poppler-qt5.h>
#include <poppler-form.h>
#include <Form.h>

class TestForms: public QObject
{
    Q_OBJECT
private slots:
    void testCheckbox();// Test for issue #655
};

void TestForms::testCheckbox()
{
    // Test for checkbox issue #655
    QScopedPointer< Poppler::Document > document(Poppler::Document::load(TESTDATADIR "/unittestcases/latex-hyperref-checkbox-issue-655.pdf"));
    QVERIFY( document );

    QScopedPointer< Poppler::Page > page(document->page(0));
    QVERIFY( page );

    QList<Poppler::FormField*> forms = page->formFields();
    QCOMPARE( forms.size(), 1 );

    Poppler::FormField *form = forms.at(0);
    QCOMPARE( form->type() , Poppler::FormField::FormButton );

    Poppler::FormFieldButton *chkFormFieldButton = static_cast<Poppler::FormFieldButton *>(form);

    // Test this is actually a Checkbox
    QCOMPARE( chkFormFieldButton->buttonType() , Poppler::FormFieldButton::CheckBox );

    // checkbox comes initially 'unchecked'
    QCOMPARE( chkFormFieldButton->state() , false );
    // let's mark it as 'checked'
    chkFormFieldButton->setState( true );
    // now test if it was succesfully 'checked'
    QCOMPARE( chkFormFieldButton->state() , true );
}

QTEST_GUILESS_MAIN(TestForms)
#include "check_forms.moc"
