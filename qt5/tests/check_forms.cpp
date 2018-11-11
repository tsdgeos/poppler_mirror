#include <QtTest/QtTest>

#include <poppler-qt5.h>
#include <poppler-form.h>
#include <Form.h>

class TestForms: public QObject
{
    Q_OBJECT
public:
    TestForms(QObject *parent = nullptr) : QObject(parent) { }
private slots:
    void testCheckbox();// Test for issue #655
    void testCheckboxIssue159();// Test for issue #159
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

void TestForms::testCheckboxIssue159()
{
    // Test for checkbox issue #159
    QScopedPointer< Poppler::Document > document(Poppler::Document::load(TESTDATADIR "/unittestcases/checkbox_issue_159.pdf"));
    QVERIFY( document );

    QScopedPointer< Poppler::Page > page(document->page(0));
    QVERIFY( page );

    Poppler::FormFieldButton *beerFieldButton = nullptr;
    Poppler::FormFieldButton *wineFieldButton = nullptr;

    QList<Poppler::FormField*> forms = page->formFields();

    // Let's find and assign the "Wine" and "Beer" radio buttons
    Q_FOREACH (Poppler::FormField *field, forms) {
        if (field->type() != Poppler::FormField::FormButton)
            continue;

        Poppler::FormFieldButton *fieldButton = static_cast<Poppler::FormFieldButton *>(field);
        if (fieldButton->buttonType() != Poppler::FormFieldButton::Radio)
            continue;

        //printf("%s \n", fieldButton->caption().toLatin1().data());
        if (fieldButton->caption() == QStringLiteral("Wine")) {
            wineFieldButton = fieldButton;
        } else if (fieldButton->caption() == QStringLiteral("Beer")) {
            beerFieldButton = fieldButton;
        }
    }

    // "Beer" and "Wine" radiobuttons belong to the same RadioButton group.
    // So selecting one should unselect the other.
    QVERIFY( beerFieldButton );
    QVERIFY( wineFieldButton );

    // Test that the RadioButton group comes with "Beer" initially selected
    QCOMPARE( beerFieldButton->state() , true );

    // Now select "Wine". As a result "Beer" should no longer be selected.
    wineFieldButton->setState(true);

    // Test that "Beer" is indeed not reporting as being selected
    QCOMPARE( beerFieldButton->state() , false );
}

QTEST_GUILESS_MAIN(TestForms)
#include "check_forms.moc"
