import java.awt.Font;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.util.Vector;

import javax.swing.AbstractAction;
import javax.swing.Action;
import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.KeyStroke;
import javax.swing.SpringLayout;
import javax.swing.SwingConstants;
import javax.swing.SwingUtilities;
import javax.swing.UIManager;
import javax.swing.UnsupportedLookAndFeelException;
import javax.swing.UIManager.LookAndFeelInfo;

class scriptAttribute {
	private int index;
	private float value;

	public scriptAttribute() {
		this.index = 0;
		this.value = 0f;
	}

	public scriptAttribute(int index, float value) {
		this.index = index;
		this.value = value;
	}

	public int getIndex() {
		return index;
	}
	public void setIndex(int index) {
		this.index = index;
	}
	public float getValue() {
		return value;
	}
	public void setValue(float value) {
		this.value = value;
	}
}

class scriptItem {
	private int index;
	private String classname;
	private String slot;
	private int quality;
	private int level;
	private Vector<scriptAttribute> attributes;

	public scriptItem() {
		this.index = 0;
		this.classname = "";
		this.slot = "";
		this.quality = 0;
		this.level = 0;
		this.attributes = new Vector<scriptAttribute>(16);
	}

	public scriptItem(int index, String classname, String slot, int quality,
			int level) {
		this.index = index;
		this.classname = classname;
		this.slot = slot;
		this.quality = quality;
		this.level = level;
		this.attributes = new Vector<scriptAttribute>(16);
	}

	public scriptItem(int index, String classname, String slot, int quality,
			int level, Vector<scriptAttribute> attributes) {
		this.index = index;
		this.classname = classname;
		this.slot = slot;
		this.quality = quality;
		this.level = level;
		this.attributes = attributes;
	}

	public int getIndex() {
		return index;
	}
	public void setIndex(int index) {
		this.index = index;
	}
	public String getClassname() {
		return classname;
	}
	public void setClassname(String classname) {
		this.classname = classname;
	}
	public String getSlot() {
		return slot;
	}
	public void setSlot(String string) {
		this.slot = string;
	}
	public int getQuality() {
		return quality;
	}
	public void setQuality(int i) {
		this.quality = i;
	}
	public int getLevel() {
		return level;
	}
	public void setLevel(int level) {
		this.level = level;
	}
	public Vector<scriptAttribute> getAttributes() {
		return attributes;
	}
	public void setAttributes(Vector<scriptAttribute> attributes) {
		this.attributes = attributes;
	}
}

@SuppressWarnings("serial")
public class custom_weps extends JPanel implements ActionListener {

	static Vector<scriptItem> itemsFound = new Vector<scriptItem>(100);

	static Vector<String> attribNames = new Vector<String>(100);
	static Vector<String> qualityNames = new Vector<String>(100);

	static JFrame frame;
	
	JButton startButton;

	JScrollPane textAreaScrollPaneLeft;
	JScrollPane textAreaScrollPaneRight;
	
	JScrollPane textAreaScrollPaneLeftTop;

	JTextArea textAreaLeft;
	JTextArea textAreaRight;
	
	JTextArea textAreaLeftTop;

	Font fixedWidth = new Font("Courier New", Font.PLAIN, 12);

	public custom_weps() {
		SpringLayout contentLayout = new SpringLayout();
		setLayout(contentLayout);
		
		KeyStroke keyShiftAltS = KeyStroke.getKeyStroke(java.awt.event.KeyEvent.VK_S, java.awt.event.InputEvent.ALT_DOWN_MASK|java.awt.event.InputEvent.SHIFT_DOWN_MASK);
		Action changeStyle = new AbstractAction() {
			public void actionPerformed(ActionEvent e) {
				if (UIManager.getLookAndFeel().getName().equals("Windows")) {
					setUI("Nimbus");
				} else if (UIManager.getLookAndFeel().getName().equals("Nimbus")) {
					setUI("Metal");
				} else if (UIManager.getLookAndFeel().getName().equals("Metal")) {
					setUI("Windows");
				}
		    	SwingUtilities.updateComponentTreeUI(frame);
		    }
		};
		getInputMap(WHEN_IN_FOCUSED_WINDOW).put(keyShiftAltS, "changeStyle");
		getActionMap().put("changeStyle", changeStyle);

		startButton = new JButton("Start Conversion");
		startButton.setVerticalTextPosition(SwingConstants.CENTER);
		startButton.setHorizontalTextPosition(SwingConstants.CENTER);
		startButton.setActionCommand("start");
		startButton.addActionListener(this);

		textAreaLeft = new JTextArea("\n1. Paste items_game.txt here.\n2. Edit formatting rule above according to your needs.\n3. Click Start Conversion.\n\n$itemindex$    <| Definition Index\n$classname$    <| Classname\n$itemslot$     <| Slot    WARNING: This is a string.\n$quality$      <| Quality\n$level$        <| Level\n\n$attribcount$  <| Attribute Count\n\n$[$            <| Attribute block.\n$attribindex$  <| Contents will be repeated\n$attribvalue$  <| for each attribute.\n$]$            <|\n\nEach format item can be repeated multiple times,\nbut the items in the attributes block only work\ninside it and vice versa.");
		textAreaLeft.setFont(fixedWidth);
		textAreaRight = new JTextArea();
		textAreaRight.setFont(fixedWidth);
		
		textAreaLeftTop = new JTextArea("\"$itemindex$\"\t// $classname$\n{\n\t\"quality\"\t\"$quality$\"\n\t\"level\"\t\t\"$level$\"\n$[$\t\"$listindex1$\"\t\t\"$attribindex$ ; $attribvalue$\"\n$]$}\n\n");
		textAreaLeftTop.setFont(fixedWidth);

		textAreaScrollPaneLeft = new JScrollPane();
		textAreaScrollPaneLeft.getViewport().add(textAreaLeft);

		textAreaScrollPaneRight = new JScrollPane();
		textAreaScrollPaneRight.getViewport().add(textAreaRight);
		
		textAreaScrollPaneLeftTop = new JScrollPane();
		textAreaScrollPaneLeftTop.getViewport().add(textAreaLeftTop);

		contentLayout.putConstraint(SpringLayout.NORTH, textAreaScrollPaneLeftTop, 5, SpringLayout.NORTH, this);
		contentLayout.putConstraint(SpringLayout.SOUTH, textAreaScrollPaneLeftTop, 180, SpringLayout.NORTH, this);
		contentLayout.putConstraint(SpringLayout.NORTH, textAreaScrollPaneLeft, 3, SpringLayout.SOUTH, textAreaScrollPaneLeftTop);
		
		contentLayout.putConstraint(SpringLayout.WEST, textAreaScrollPaneLeftTop, 5, SpringLayout.WEST, this);
		contentLayout.putConstraint(SpringLayout.EAST, textAreaScrollPaneLeftTop, -2, SpringLayout.HORIZONTAL_CENTER, this);
		
		contentLayout.putConstraint(SpringLayout.SOUTH, textAreaScrollPaneLeft, -3, SpringLayout.NORTH, startButton);
		contentLayout.putConstraint(SpringLayout.NORTH, textAreaScrollPaneRight, 5, SpringLayout.NORTH, this);
		contentLayout.putConstraint(SpringLayout.SOUTH, textAreaScrollPaneRight, -3, SpringLayout.NORTH, startButton);

		contentLayout.putConstraint(SpringLayout.WEST, textAreaScrollPaneLeft, 5, SpringLayout.WEST, this);
		contentLayout.putConstraint(SpringLayout.EAST, textAreaScrollPaneRight, -5, SpringLayout.EAST, this);

		contentLayout.putConstraint(SpringLayout.EAST, textAreaScrollPaneLeft, -2, SpringLayout.HORIZONTAL_CENTER, this);
		contentLayout.putConstraint(SpringLayout.WEST, textAreaScrollPaneRight, 2, SpringLayout.HORIZONTAL_CENTER, this);

		contentLayout.putConstraint(SpringLayout.SOUTH, startButton, -4, SpringLayout.SOUTH, this);

		contentLayout.putConstraint(SpringLayout.EAST, startButton, -4, SpringLayout.EAST, this);
		contentLayout.putConstraint(SpringLayout.WEST, startButton, 4, SpringLayout.WEST, this);

		contentLayout.putConstraint(SpringLayout.NORTH, startButton, -50, SpringLayout.SOUTH, startButton);

		add(textAreaScrollPaneLeft);
		add(textAreaScrollPaneRight);
		add(textAreaScrollPaneLeftTop);
		add(startButton);
	}

	public static void main(String[] args) {
		setUI("Windows");
		javax.swing.SwingUtilities.invokeLater(new Runnable() {
			public void run() {
				createAndShowGUI();
			}
		});
	}

	private static void setUI(String laf) {
		for (LookAndFeelInfo info : UIManager.getInstalledLookAndFeels()) {
			if (laf.equals(info.getName())) {
				try {
					UIManager.setLookAndFeel(info.getClassName());
				} catch (ClassNotFoundException e) {
					continue;
				} catch (InstantiationException e) {
					continue;
				} catch (IllegalAccessException e) {
					continue;
				} catch (UnsupportedLookAndFeelException e) {
					continue;
				}
				break;
			}
		}
	}

	public void convertItems() {
		itemsFound.removeAllElements();
		attribNames.removeAllElements();

		String content = textAreaLeft.getText().toLowerCase().replaceAll("[\r\n\t ]", "");
		
		creatQualityVector(content);
		creatAttribVector(content);
		creatItemVector(content);
	}
	
	private void creatAttribVector(String content) {
		attribNames.add("padding");
		
		int matchStart = 0;
		int matchEnd = 0;
		int searchIndex = 0;
		
		if ((matchStart = content.indexOf("\"attributes\"{\"1\"", searchIndex)) == -1)
			return;
		matchStart += 13;
			
		if ((matchEnd = content.indexOf("}}", matchStart)) == -1)
			return;
		matchEnd += 1;
		
		content = content.substring(matchStart, matchEnd);
		
		while (true) {
			if ((matchStart = content.indexOf("\"name\"", searchIndex)) == -1)
				break;
			matchStart += 7;
				
			if ((matchEnd = content.indexOf("\"", matchStart)) == -1)
				break;
			
			searchIndex = matchEnd;
			
			attribNames.add(content.substring(matchStart, matchEnd));
		}
	}
	
	private void creatQualityVector(String content) {
		int matchStart = 0;
		int matchEnd = 0;
		int searchIndex = 0;
		
		if ((matchStart = content.indexOf("\"qualities\"{\"", searchIndex)) == -1)
			return;
		matchStart += 11;
			
		if ((matchEnd = content.indexOf("}}\"items\"", matchStart)) == -1)
			return;
		matchEnd += 1;

		content = content.substring(matchStart, matchEnd);
		
		while (true) {
			if ((matchEnd = content.indexOf("\"{\"value\"", searchIndex)) == -1)
				break;
				
			if ((matchStart = content.lastIndexOf("\"", matchEnd-1)) == -1)
				break;
			matchStart += 1;
			
			qualityNames.add(content.substring(matchStart, matchEnd));
			
			/* matchStart = matchEnd+10;
			
			if ((matchEnd = content.indexOf("\"", matchStart)) == -1)
				break;
			
			int value = Integer.parseInt(content.substring(matchStart, matchEnd)); */
			
			searchIndex = matchEnd+1;
		}
	}

	private void creatItemVector(String content) {
		int matchStart = 0;
		int matchEnd = 0;
		int searchIndex = 0;
		int braceCount = 0;
		char tempBrace;
		boolean atLeastOne = false;
		scriptItem temp;
		
		String singleItem = "";
		
		if ((matchStart = content.indexOf("\"items\"{\"0\"", searchIndex)) == -1)
			return;
		matchStart += 8;
			
		if ((matchEnd = content.indexOf("}}\"attributes\"", matchStart)) == -1)
			return;
		matchEnd += 1;
		
		content = content.substring(matchStart, matchEnd);
		
		while (true) {
			braceCount = 0;
			atLeastOne = false;
			
			if ((matchStart = content.indexOf("\"", searchIndex)) == -1)
				break;
			matchStart += 1;
			
			for (int c = matchStart; c < content.length(); c++){
				tempBrace = content.charAt(c);
				if (tempBrace == '{') {
					braceCount++;
					atLeastOne = true;
				}
				else if (tempBrace == '}')
					braceCount--;
				if (braceCount == 0 && atLeastOne) {
					matchEnd = c;
					break;
				}
			}
			
			singleItem = content.substring(matchStart, matchEnd);
			
			searchIndex = matchEnd;
			
			if ((temp = parseItem(singleItem)) != null)
				itemsFound.add(temp);
		}
	}
	
	private scriptItem parseItem(String content) {
		int matchStart = 0;
		int matchEnd = 0;
		int searchIndex = 0;
		
		scriptItem tempItem = new scriptItem();
		Vector<scriptAttribute> tempAttribCollection;
			
		if ((matchEnd = content.indexOf("\"", searchIndex)) == -1)
			return null;
		
		searchIndex = matchEnd+1;
		
		tempItem.setIndex(Integer.parseInt(content.substring(matchStart, matchEnd)));
		
		if ((matchStart = content.indexOf("\"item_class\"", searchIndex)) == -1)
			return null;
		matchStart += 13;
			
		if ((matchEnd = content.indexOf("\"", matchStart)) == -1)
			return null;
		
		searchIndex = matchEnd;
		
		tempItem.setClassname(content.substring(matchStart, matchEnd));
		
		if ((matchStart = content.indexOf("\"item_slot\"", searchIndex)) == -1)
			return null;
		matchStart += 12;
			
		if ((matchEnd = content.indexOf("\"", matchStart)) == -1)
			return null;
		
		searchIndex = matchEnd;
		
		tempItem.setSlot(content.substring(matchStart, matchEnd));
		
		if ((matchStart = content.indexOf("\"item_quality\"", searchIndex)) == -1)
			return null;
		matchStart += 15;
			
		if ((matchEnd = content.indexOf("\"", matchStart)) == -1)
			return null;
		
		searchIndex = matchEnd;
		
		tempItem.setQuality(qualityNames.indexOf(content.substring(matchStart, matchEnd)));
		
		if ((matchStart = content.indexOf("\"min_ilevel\"", searchIndex)) == -1)
			return null;
		matchStart += 13;
			
		if ((matchEnd = content.indexOf("\"", matchStart)) == -1)
			return null;
		
		searchIndex = matchEnd;
		
		tempItem.setLevel(Integer.parseInt(content.substring(matchStart, matchEnd)));
		
		if ((tempAttribCollection = parseAttributes(content)) != null)
			tempItem.setAttributes(tempAttribCollection);
		
		//if (!tempItem.getClassname().equals("tf_wearable_item") && !tempItem.getClassname().equals("slot_token") && tempItem.getQuality() != 0)
			return tempItem;
		//else
		//	return null;
	}
	
	private Vector<scriptAttribute> parseAttributes(String content) {
		int matchStart = 0;
		int matchEnd = 0;
		int searchIndex = 0;
		Vector<scriptAttribute> tempAttribCollection = new Vector<scriptAttribute>(16);
		scriptAttribute tempAttribute;
		
		if ((matchStart = content.indexOf("\"attributes\"{", searchIndex)) == -1)
			return null;
		matchStart += 13;
			
		if ((matchEnd = content.indexOf("}}", matchStart)) == -1)
			return null;
		matchEnd += 1;
		
		content = content.substring(matchStart, matchEnd);
		
		while (true) {
			tempAttribute = new scriptAttribute();
			
			if ((matchStart = content.indexOf("\"", searchIndex)) == -1)
				break;
			matchStart += 1;
			
			if ((matchEnd = content.indexOf("\"{", matchStart)) == -1)
				break;
			
			searchIndex = matchEnd+1;

			tempAttribute.setIndex(attribNames.indexOf(content.substring(matchStart, matchEnd)));
			
			if ((matchStart = content.indexOf("\"value\"\"", searchIndex)) == -1)
				break;
			matchStart += 8;
			
			if ((matchEnd = content.indexOf("\"", matchStart)) == -1)
				break;
			
			if ((searchIndex = content.indexOf("}", matchEnd)) == -1)
				break;
			
			tempAttribute.setValue(Float.parseFloat(content.substring(matchStart, matchEnd)));
			
			tempAttribCollection.add(tempAttribute);
		}
		
		return tempAttribCollection;
	}

	private static void createAndShowGUI() {
		frame = new JFrame("Converter");
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

		custom_weps newContentPane = new custom_weps();
		newContentPane.setOpaque(true);
		frame.setContentPane(newContentPane);

		frame.setSize(900, 600);
		frame.setVisible(true);
	}

	@Override
	public void actionPerformed(ActionEvent e) {
		if (e.getActionCommand().equals("start")) {
			textAreaRight.setText("");
			textAreaRight.setEnabled(false);
			textAreaLeft.setEnabled(false);
			textAreaLeftTop.setEnabled(false);
			startButton.setEnabled(false);
			
			convertItems();
			
			startButton.setEnabled(true);
			textAreaRight.setEnabled(true);
			textAreaLeft.setEnabled(true);
			textAreaLeftTop.setEnabled(true);
			
			textAreaRight.requestFocusInWindow();
			
/*
"$itemindex$"
{
	"quality"	"$quality$"
	"level"		"$level$"
	"attrib_count"	"$attribcount$"
	"attributes"
	{
$[$		"$attribindex$"	"$attribvalue$"
$]$	}
}

*/
			String format = textAreaLeftTop.getText();
			String attribOutput = "";
			String output = "";
			String attribFormat ="";
			
			int matchStart = 0;
			int matchEnd = 0;
			
			matchStart = format.indexOf("$[$");
			matchStart += 3;
			matchEnd = format.indexOf("$]$", matchStart);
			
			attribFormat = format.substring(matchStart, matchEnd);
			
			scriptItem tempItem;
			scriptAttribute tempAttrib;
			for (int i = 0; i < itemsFound.size(); i++) {
				tempItem = itemsFound.elementAt(i);
				
				output = format;
				output = output.replaceAll("\\$itemindex\\$", tempItem.getIndex()+"");
				output = output.replaceAll("\\$classname\\$", tempItem.getClassname()+"");
				output = output.replaceAll("\\$itemslot\\$", tempItem.getSlot()+"");
				output = output.replaceAll("\\$quality\\$", tempItem.getQuality()+"");
				output = output.replaceAll("\\$level\\$", tempItem.getLevel()+"");
				
				output = output.replaceAll("\\$attribcount\\$", tempItem.getAttributes().size()+"");
				
				attribOutput = "";
				for (int a = 0; a < tempItem.getAttributes().size(); a++) {
					tempAttrib = tempItem.getAttributes().elementAt(a);
					attribOutput = attribOutput + attribFormat;
					attribOutput = attribOutput.replaceAll("\\$attribindex\\$", tempAttrib.getIndex()+"");
					attribOutput = attribOutput.replaceAll("\\$attribvalue\\$", tempAttrib.getValue()+"");
					attribOutput = attribOutput.replaceAll("\\$listindex\\$", a+"");
					attribOutput = attribOutput.replaceAll("\\$listindex1\\$", (a+1)+"");
				}
				
				output = output.replaceAll("\\$\\[\\$[\\s\\S]+\\$\\]\\$", attribOutput+"");
				
				textAreaRight.setText(textAreaRight.getText()+output);		
			}
		}
	}	
}