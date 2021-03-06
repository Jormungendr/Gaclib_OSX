//
//  listview_sorting_and_filtering.h
//  GacOSX
//
//  Created by Robert Bu on 12/14/14.
//  Copyright (c) 2014 Robert Bu. All rights reserved.
//

#ifndef GacOSX_listview_sorting_and_filtering_h
#define GacOSX_listview_sorting_and_filtering_h

#include "../shared/gac_include.h"
#include "../shared/osx_shared.h"
#include "../shared/UnixFileSystemInfo.h"

class SortingAndFilteringWindow : public GuiWindow
{
private:
    GuiListView*                    listView;
    GuiToolstripMenu*               fileTypeMenu;
    List<Ptr<FileProperties>>       fileProperties;
    
    vint                            sortingColumn;
    bool                            ascending;
    WString                         filteredFileType;
    
    vint ItemComparer(Ptr<FileProperties> a, Ptr<FileProperties> b)
    {
        int result=0;
        switch(sortingColumn)
        {
            case -1:
                return fileProperties.IndexOf(a.Obj())-fileProperties.IndexOf(b.Obj());
            case 0:
                result=wcsicmp(a->GetDisplayName().Buffer(), b->GetDisplayName().Buffer());
                break;
            case 1:
                result=wcsicmp(a->GetTypeName().Buffer(), b->GetTypeName().Buffer());
                break;
            case 2:
            {
                vint64_t la= a->GetLastWriteTime().tv_sec;
                vint64_t lb = b->GetLastWriteTime().tv_sec;
                result = la > lb ? 1 : la < lb ? -1 : 0;
            }
                break;
            case 3:
            {
                vint64_t la = a->GetSize();
                vint64_t lb = b->GetSize();
                result = la > lb ? 1 : la < lb ? -1 : 0;
            }
                break;
        }
        if(!ascending) result*=-1;
        return result;
    }
    
    bool ItemFilter(Ptr<FileProperties> a)
    {
        return filteredFileType==L"" || a->GetTypeName()==filteredFileType;
    }
    
    Ptr<list::ListViewItem> CreateFileItem(Ptr<FileProperties> file)
    {
        Ptr<list::ListViewItem> item=new list::ListViewItem;
        
        // Get large icon.
        item->SetLargeImage(file->GetBigIcon());
        // Get small icon.
        item->SetSmallImage(file->GetSmallIcon());
        // Get display name
        item->SetText(file->GetDisplayName());
        // Get type name
        item->GetSubItems().Add(file->GetTypeName());
        // Get last write time
        item->GetSubItems().Add(FileModificationTimeToString(file->GetLastWriteTime()));
        // Get file size
        item->GetSubItems().Add(GetFileSizeString(file->GetSize()));
        
        return item;
    }
    
    void FillData()
    {
        listView->GetItems().Clear();
        CopyFrom(listView->GetItems(),
                 From(fileProperties)
                 .Where([this](Ptr<FileProperties> a){return ItemFilter(a);})
                 .OrderBy([this](Ptr<FileProperties> a, Ptr<FileProperties> b){return ItemComparer(a, b);})
                 .Select([this](Ptr<FileProperties> a){return CreateFileItem(a);})
                 );
    }
    
    void ShowAllFileType_Clicked(GuiGraphicsComposition* sender, GuiEventArgs& arguments)
    {
        filteredFileType=L"";
        FillData();
    }
    
    void ShowSingleFileType_Clicked(GuiGraphicsComposition* sender, GuiEventArgs& arguments)
    {
        filteredFileType=sender->GetRelatedControl()->GetText();
        FillData();
    }
    
    void Column_Clicked(GuiGraphicsComposition* sender, GuiItemEventArgs& arguments)
    {
        // Get the sorting state of the clicked column
        GuiListViewColumnHeader::ColumnSortingState state=listView->GetItems().GetColumns()[arguments.itemIndex]->GetSortingState();
        
        // Calculate the new sorting state
        switch(state)
        {
            case GuiListViewColumnHeader::NotSorted:
                sortingColumn=arguments.itemIndex;
                ascending=true;
                break;
            case GuiListViewColumnHeader::Ascending:
                sortingColumn=arguments.itemIndex;
                ascending=false;
                break;
            case GuiListViewColumnHeader::Descending:
                sortingColumn=-1;
                break;
        }
        
        // Update all list view columns
        for(int i=0;i<listView->GetItems().GetColumns().Count();i++)
        {
            Ptr<list::ListViewColumn> column=listView->GetItems().GetColumns()[i];
            if(i==sortingColumn)
            {
                column->SetSortingState(ascending?GuiListViewColumnHeader::Ascending:GuiListViewColumnHeader::Descending);
            }
            else
            {
                column->SetSortingState(GuiListViewColumnHeader::NotSorted);
            }
        }
        listView->GetItems().GetColumns().NotifyUpdate(0, listView->GetItems().GetColumns().Count());
        
        // Refresh data;
        FillData();
    }
public:
    SortingAndFilteringWindow()
    :GuiWindow(GetCurrentTheme()->CreateWindowStyle())
    ,sortingColumn(-1)
    ,ascending(true)
    {
        this->SetText(L"Controls.ListView.SortingAndFiltering");
        
        // Create list view control to display a folder
        listView=g::NewListViewBigIcon();
        listView->SetHorizontalAlwaysVisible(false);
        listView->SetVerticalAlwaysVisible(false);
        listView->SetMultiSelect(true);
        listView->GetBoundsComposition()->SetAlignmentToParent(Margin(4, 4, 4, 4));
        this->AddChild(listView);
        
        // Add columns and changed to detail view
        listView->GetItems().GetColumns().Add(new list::ListViewColumn(L"Name", 230));
        listView->GetItems().GetColumns().Add(new list::ListViewColumn(L"Type", 120));
        listView->GetItems().GetColumns().Add(new list::ListViewColumn(L"Date", 120));
        listView->GetItems().GetColumns().Add(new list::ListViewColumn(L"Size", 120));
        listView->ChangeItemStyle(new list::ListViewDetailContentProvider);
        // Register the column clicked event
        listView->ColumnClicked.AttachMethod(this, &SortingAndFilteringWindow::Column_Clicked);
        
        {
            // Enumerate all directories and files in the Windows directory.
            WString directory=GetUserHome();
            List<WString> directories;
            List<WString> files;
            SearchDirectoriesAndFiles(directory, directories, files);
            FOREACH(WString, file, From(directories).Concat(files))
            {
                fileProperties.Add(new FileProperties(directory+L"/"+file));
            }
        }
        {
            // Create a popup menu
            fileTypeMenu=g::NewMenu(0);
            GuiToolstripButton* button=0;
            
            // Create "Show all file types" menu
            fileTypeMenu->GetBuilder()->Button(0, L"Show all file types", &button);
            button->Clicked.AttachMethod(this, &SortingAndFilteringWindow::ShowAllFileType_Clicked);
            // Create menu separator
            fileTypeMenu->GetBuilder()->Splitter();
            
            // Added all existing file type in the folder as menu items
            Array<WString> fileTypes;
            CopyFrom(
                     fileTypes,
                     From(fileProperties)
                     .Select([](Ptr<FileProperties> file){return file->GetTypeName();})
                     .Distinct()
                     .OrderBy([](WString a, WString b){return wcsicmp(a.Buffer(), b.Buffer());})
                     );
            FOREACH(WString, typeName, fileTypes)
            {
                // Create menu button for each file type
                fileTypeMenu->GetBuilder()->Button(0, typeName, &button);
                button->Clicked.AttachMethod(this, &SortingAndFilteringWindow::ShowSingleFileType_Clicked);
            }
            
            // Attach the menu to the "Type" column
            listView->GetItems().GetColumns()[1]->SetDropdownPopup(fileTypeMenu);
            listView->GetItems().GetColumns().NotifyUpdate(1);
        }
        FillData();
        
        // set the preferred minimum client size
        this->GetBoundsComposition()->SetPreferredMinSize(Size(640, 480));
        // call this to calculate the size immediately if any indirect content in the table changes
        // so that the window can calcaulte its correct size before calling the MoveToScreenCenter()
        this->ForceCalculateSizeImmediately();
        // move to the screen center
        this->MoveToScreenCenter();
    }
    
    ~SortingAndFilteringWindow()
    {
        listView->GetItems().GetColumns()[1]->SetDropdownPopup(0);
        listView->GetItems().GetColumns().NotifyUpdate(1);
        delete fileTypeMenu;
    }
};


int main(int argc, const char * argv[])
{
    SetupOSXCoreGraphicsRenderer();
    
    return 0;
}

void GuiMain()
{
    RunGacWindow<SortingAndFilteringWindow>();
}

#endif
