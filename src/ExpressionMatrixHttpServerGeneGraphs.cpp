// Http server functionality related to gene graphs.

#include "ExpressionMatrix.hpp"
#include "GeneGraph.hpp"
using namespace ChanZuckerberg;
using namespace ExpressionMatrix2;



void ExpressionMatrix::exploreGeneGraphs(const vector<string>& request, ostream& html)
{
    html << "<h1>Gene graphs</h1>";

    // Table of existing gene graphs.
    html << "<h2>Existing gene graphs</h2><table>";
    for(const auto& p: geneGraphs) {
        html << "<tr><td><a href='exploreGeneGraph?geneGraphName="
            << p.first << "'>" << p.first << "</a>"
            "<td><form action=removeGeneGraph>"
            "<input type=submit value='Remove'>"
            "<input hidden type=text name=signatureGeneName value='" << p.first << "'>"
            "</form>";
    }
    html << "</table>";

    // Form to create a new gene graph.
    html <<
        "<h2>Create a new gene graph</h2>"
        "<form action=createGeneGraph>"
        "<input type=submit value='Create'> a new gene graph named "
        "<input type=text size=8 name=geneGraphName required> "
        "for gene set ";
    writeGeneSetSelection(html, "geneSetName", false);
    html << "</form>";
}



void ExpressionMatrix::exploreGeneGraph(const vector<string>& request, ostream& html)
{
    // Get parameters from the request.
    string geneGraphName;
    getParameterValue(request, "geneGraphName", geneGraphName);
    if(geneGraphName.empty()) {
        html << "Gene graph name is missing.";
        return;
    }
    GeneGraph& geneGraph = getGeneGraph(geneGraphName);


    GeneGraph::SvgParameters svgParameters;
    string hideEdges;
    getParameterValue(request, "hideEdges", hideEdges);
    svgParameters.hideEdges = (hideEdges == "on");
    getParameterValue(request, "svgSizePixels", svgParameters.svgSizePixels);
    getParameterValue(request, "xShift", svgParameters.xShift);
    getParameterValue(request, "yShift", svgParameters.yShift);
    getParameterValue(request, "zoomFactor", svgParameters.zoomFactor);
    getParameterValue(request, "vertexSizeFactor", svgParameters.vertexSizeFactor);
    getParameterValue(request, "edgeThicknessFactor", svgParameters.edgeThicknessFactor);

    string metaDataMeaning = "category";
    getParameterValue(request, "metaDataMeaning", metaDataMeaning);



    html << "<h2>Gene graph " << geneGraphName << "</h2>";





    // Write out the JavaScript and html to allow manipulating the svg.
    html << R"###(
<br>
To pan  the graphics, use the mouse to drag an empty location in the graph.
<br>
<form id=mouseWheelForm>
Mouse wheel controls: 
<input id=mouseWheelFormDefault type="radio" onclick='updateMouseWheelFunction(this)' name="mouseWheelFunction" value="zoom" checked=checked>zoom 
<input type="radio" onclick='updateMouseWheelFunction(this)' name="mouseWheelFunction" value="graphicsSize">graphics size 
<input type="radio" onclick='updateMouseWheelFunction(this)' name="mouseWheelFunction" value="vertexSize">vertex size 
<input type="radio" onclick='updateMouseWheelFunction(this)' name="mouseWheelFunction" value="edgeThickness">edge thickness
</form>



<script>
document.getElementById("mouseWheelFormDefault").checked = true;
// Function called when the user clicks one of the radio buttons
// that control mouse wheel function.
var mouseWheelFunction = "zoom";
function updateMouseWheelFunction(element) 
{
    if(element.checked) {
        mouseWheelFunction = element.value;
    }
}

// Turn off scrolling with the mouse wheel.
// Otherwise it interacts with this page's use of the mouse wheel.
// This is browser dependent. As coded, works 
// in Firefox 58.0 and Chrome 64.0.
function stopWheel(e){
    if(!e) { 
        e = window.event; 
    } 
    e.stopPropagation();
    e.preventDefault();
    e.cancelBubble = false;
    return false;
}
document.addEventListener('DOMMouseScroll', stopWheel, false);
document.addEventListener('wheel', stopWheel, false);

var mouseDown = false;
var xMouse = 0;
var yMouse = 0;


// Set the viewBox of the svg object based on the current values of the global variables.
function setViewBox()
{
    var svg = document.getElementById("svgObject");
    var xMin = xCenter-halfViewBoxSize;
    var yMin = yCenter-halfViewBoxSize;
    var viewBox = xMin + " " + yMin + " " + 2.*halfViewBoxSize + " " + 2.*halfViewBoxSize;
    svg.setAttribute("viewBox",  viewBox);
}


// Extract the delta in pixels from a wheel event.
// The constant is set in such a way that Firefox and Chrome
// behave in approximately the same way.
function extractWheelDelta(e) {
    if (e.deltaMode == WheelEvent.DOM_DELTA_LINE) {
        return e.deltaY * 27;
    } else {
        return e.deltaY;
    }
}

function mouseDownHandler(event) {
    // alert("mouseDownHandler");
    xMouse = event.clientX;
    yMouse = event.clientY;
    mouseDown = true;
}
function mouseMoveHandler(event) {
    if(mouseDown) {
        // alert("mouseMoveHandler");
        var xDelta = event.clientX - xMouse;
        var yDelta = event.clientY - yMouse;
        xCenter -= pixelSize*xDelta;
        yCenter -= pixelSize*yDelta;
        xShift += pixelSize*xDelta;
        yShift += pixelSize*yDelta;
        xMouse = event.clientX;
        yMouse = event.clientY;
        setViewBox();    
    }
}
function mouseUpHandler() {
    // alert("mouseUpHandler");
    mouseDown = false;
}



// Function called when the user moves the mouse wheel.
function handleMouseWheelEvent(e) 
{
    if(mouseWheelFunction == "zoom") {
        zoomHandler(e);
    } else if(mouseWheelFunction == "graphicsSize") {
        handleGraphicsResizeEvent(e);
    } else if(mouseWheelFunction == "vertexSize") {
        handleVertexResizeEvent(e);
    } else if(mouseWheelFunction == "edgeThickness") {
        handleEdgeThicknessEvent(e);
    }
}


// Function called when the user moves the mouse wheel to resize the vertices.
function handleVertexResizeEvent(e) {
    var delta = extractWheelDelta(e);
    var factor = Math.exp(-.001*delta);
    vertexSizeFactor *= factor;
    
    // Resize all the vertices.
    var vertices = document.getElementById("vertices").childNodes;
    for(i=0; i<vertices.length; i++) {
        vertex = vertices[i];
        if(vertex.tagName == "circle" || vertex.tagName == "path") {
            // vertex.setAttribute("r", factor*vertex.getAttribute("r")); This only works for circles.
            vertex.transform.baseVal.getItem(1).setScale(vertexSizeFactor, vertexSizeFactor); 
        }
    }
}



// Function called when the user moves the mouse wheel to change edge thickness.
function handleEdgeThicknessEvent(e) {
    var delta = extractWheelDelta(e);
    var factor = Math.exp(-.001*delta);
    edgeThicknessFactor *= factor;
    // alert(edgeThicknessFactor);
    
    
    // Change the thickness of all the edges.
    var edges = document.getElementById("edges").childNodes;
    for(i=0; i<edges.length; i++) {
        if(edges[i].tagName == "line") {
            edges[i].style.strokeWidth *= factor;
        }
    }
}



// Function called when the user moves the mouse wheel to zoom in or out.
function zoomHandler(e) {
    var delta = extractWheelDelta(e);
    var factor = Math.exp(.001*delta);
    zoomFactor /= factor;
    halfViewBoxSize *= factor;
    pixelSize *= factor;
    setViewBox();
}



// Function called when the user moves the mouse wheel to resize the svg graphics.
function handleGraphicsResizeEvent(e) {
    var delta = extractWheelDelta(e);
    var factor = Math.exp(-.001*delta);
    svgSizePixels *= factor;
    pixelSize /= factor;
    
    svg = document.getElementById("svgObject");
    svg.setAttribute("width", factor*svg.getAttribute("width"));
    svg.setAttribute("height", factor*svg.getAttribute("height"));
}


// Function called when the user clicks "Redraw graph."
function prepareColoringFormForSubmit()
{
    document.getElementById("svgSizePixels").value = svgSizePixels;
    document.getElementById("xShift").value = xShift;
    document.getElementById("yShift").value = yShift;
    document.getElementById("zoomFactor").value = zoomFactor;
    document.getElementById("vertexSizeFactor").value = vertexSizeFactor;
    document.getElementById("edgeThicknessFactor").value = edgeThicknessFactor;
}
</script>
)###";



    // Write the svg.
    html <<
        "<div style='float:left;' "
        "onmousedown='mouseDownHandler(event);' "
        "onmouseup='mouseUpHandler(event);' "
        "onmousemove='mouseMoveHandler(event);' "
        "onwheel='handleMouseWheelEvent(event);'>";
    geneGraph.writeSvg(html, svgParameters);
    html << "</div>";

    // Svg display parameters get written to the html in Javascript code
    html <<
        "<script>"
        "var svgSizePixels = " << svgParameters.svgSizePixels << ";"
        "var xShift = " << svgParameters.xShift << ";"
        "var yShift = " << svgParameters.yShift << ";"
        "var zoomFactor = " << svgParameters.zoomFactor << ";"
        "var vertexSizeFactor = " << svgParameters.vertexSizeFactor << ";"
        "var edgeThicknessFactor = " << svgParameters.edgeThicknessFactor << ";"
        "var xCenter = " << svgParameters.xCenter << ";"
        "var yCenter = " << svgParameters.yCenter << ";"
        "var halfViewBoxSize = " << svgParameters.halfViewBoxSize << ";"
        "var pixelSize = " << svgParameters.pixelSize << ";"
        "</script>";
}



void ExpressionMatrix::createGeneGraph(const vector<string>& request, ostream& html)
{
    string geneGraphName;
    getParameterValue(request, "geneGraphName", geneGraphName);
    if(geneGraphName.empty()) {
        html << "Gene graph name is missing.";
        return;
    }

    string geneSetName;
    getParameterValue(request, "geneSetName", geneSetName);
    if(geneSetName.empty()) {
        html << "Gene set name is missing.";
        return;
    }


    html << "<h1>Create gene graph " << geneGraphName << "</h1>";
    CZI_ASSERT(0);
    html << "<p>Gene graph " << geneGraphName << " was created."
        "<p><form action=exploreGeneGraph>"
        "<input type=text hidden name=geneGraphName value=" << geneGraphName <<
        "><input type=submit value=Continue></form>";

}



void ExpressionMatrix::removeGeneGraph(const vector<string>& request, ostream& html)
{
    string geneGraphName;
    getParameterValue(request, "geneGraphName", geneGraphName);
    if(geneGraphName.empty()) {
        html << "Gene graph name is missing.";
        return;
    }
    removeSignatureGraph(geneGraphName);

    html << "<p>Gene graph " << geneGraphName << " was removed."
        "<p><form action=exploreGeneGraphs>"
        "<input type=submit value=Continue></form>";

}
